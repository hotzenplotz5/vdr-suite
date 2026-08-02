#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import sqlite3
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
INNER_RUNNER = Path(__file__).with_name(
    "protected-mutation-outcome-runner.py"
)
DEFAULT_SERVICE = "vdr-suite-daemon.service"
DEFAULT_INSTALLED_DAEMON = Path("/usr/sbin/vdr-suite-daemon")
DEFAULT_CANDIDATE_DAEMON = REPOSITORY_ROOT / "build/vdr-suite-daemon"
DEFAULT_LOADER = Path(
    "/usr/share/vdr-suite/web/frontend/platform/deferred-runtime-loader.js"
)
DEFAULT_CONFIGURATION = Path("/etc/default/vdr-suite-daemon")
DEFAULT_DATABASE = Path("/var/lib/vdr-suite/vdr-suite.db")
SYSTEMD_RUNTIME_ROOT = Path("/run/systemd/system")
DROP_IN_NAME = "phase62-slice2x-runtime-acceptance.conf"
SERVICE_PATTERN = re.compile(r"[A-Za-z0-9_.@:-]+")


class AcceptanceError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AcceptanceError(message)


def run(
    *arguments: str,
    cwd: Path = REPOSITORY_ROOT,
    check: bool = True,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        arguments,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and completed.returncode != 0:
        command = arguments[0] if arguments else "unknown"
        raise AcceptanceError(f"command_failed:{command}")
    return completed


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def database_logical_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    database = sqlite3.connect(
        f"file:{path}?mode=ro",
        uri=True,
        timeout=10,
    )
    try:
        database.execute("PRAGMA busy_timeout=10000")
        require(
            database.execute("PRAGMA quick_check").fetchone()[0] == "ok",
            "database_quick_check_failed",
        )
        require(
            not database.execute("PRAGMA foreign_key_check").fetchall(),
            "database_foreign_key_check_failed",
        )
        for statement in database.iterdump():
            digest.update(statement.encode("utf-8"))
            digest.update(b"\n")
    finally:
        database.close()
    return digest.hexdigest()


def backup_database(source: Path, destination: Path) -> None:
    source_database = sqlite3.connect(
        f"file:{source}?mode=ro",
        uri=True,
        timeout=10,
    )
    destination_database = sqlite3.connect(str(destination))
    try:
        source_database.execute("PRAGMA busy_timeout=10000")
        source_database.backup(destination_database)
    finally:
        destination_database.close()
        source_database.close()
    os.chmod(destination, 0o600)


def atomic_install(source: Path, destination: Path) -> None:
    temporary = destination.with_name(destination.name + ".slice2x-new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "candidate_install_temporary_exists",
    )
    shutil.copyfile(source, temporary)
    os.chmod(temporary, 0o755)
    os.chown(temporary, 0, 0)
    os.replace(temporary, destination)


def restore_exact(source: Path, destination: Path) -> None:
    temporary = destination.with_name(destination.name + ".slice2x-restore")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "daemon_restore_temporary_exists",
    )
    shutil.copyfile(source, temporary)
    os.chmod(temporary, stat.S_IMODE(source.stat().st_mode))
    os.chown(temporary, 0, 0)
    os.replace(temporary, destination)


def service_pid(service: str) -> int:
    value = run(
        "systemctl",
        "show",
        "--property=MainPID",
        "--value",
        service,
    ).stdout.strip()
    return int(value or "0")


def stop_service(service: str) -> None:
    run("systemctl", "stop", service)
    deadline = time.monotonic() + 20.0
    last_state = "unknown"
    last_pid = -1
    while time.monotonic() < deadline:
        last_state = run(
            "systemctl",
            "is-active",
            service,
            check=False,
        ).stdout.strip()
        last_pid = service_pid(service)
        if last_state in ("inactive", "failed", "unknown") and last_pid == 0:
            return
        time.sleep(0.1)
    raise AcceptanceError(
        f"service_did_not_stop:state={last_state}:pid={last_pid}"
    )


def wait_service(service: str) -> int:
    deadline = time.monotonic() + 20.0
    last_state = "unknown"
    last_pid = 0
    while time.monotonic() < deadline:
        last_state = run(
            "systemctl",
            "is-active",
            service,
            check=False,
        ).stdout.strip()
        last_pid = service_pid(service)
        if last_state == "active" and last_pid > 0:
            return last_pid
        time.sleep(0.1)
    raise AcceptanceError(
        f"service_did_not_start:state={last_state}:pid={last_pid}"
    )


def process_environment(pid: int) -> set[bytes]:
    return set(Path(f"/proc/{pid}/environ").read_bytes().split(b"\0"))


def override_path(service: str) -> Path:
    require(
        SERVICE_PATTERN.fullmatch(service) is not None,
        "invalid_systemd_service_name",
    )
    return SYSTEMD_RUNTIME_ROOT / f"{service}.d" / DROP_IN_NAME


def environment_line(key: str, value: str) -> str:
    require(
        "\x00" not in value and "\r" not in value and "\n" not in value,
        "invalid_systemd_environment_value",
    )
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'Environment="{key}={escaped}"\n'


def render_override(database: Path) -> str:
    require(database.is_absolute(), "scenario_database_must_be_absolute")
    value = str(database)
    return (
        "[Service]\n"
        + environment_line("VDR_SUITE_DATABASE_PATH", value)
        + environment_line("VDR_SUITE_SECURITY_DATABASE_PATH", value)
    )


def write_override(path: Path, database: Path) -> None:
    require(not path.exists() and not path.is_symlink(), "runtime_override_exists")
    path.parent.mkdir(mode=0o755, parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "runtime_override_temporary_exists",
    )
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    descriptor = os.open(temporary, flags, 0o644)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
            descriptor = -1
            handle.write(render_override(database))
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        if temporary.exists() or temporary.is_symlink():
            temporary.unlink()


def remove_override(path: Path) -> None:
    if path.exists() or path.is_symlink():
        require(not path.is_dir(), "runtime_override_is_directory")
        path.unlink()
    try:
        path.parent.rmdir()
    except OSError:
        pass


def write_checksums(directory: Path, files: list[Path]) -> None:
    lines = [
        f"{sha256(path)}  {path.name}"
        for path in sorted(files, key=lambda item: item.name)
    ]
    (directory / "SHA256SUMS").write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def copy_optional(source: Path, destination: Path) -> bool:
    if not source.is_file():
        return False
    shutil.copy2(source, destination)
    return True


def preflight(arguments: argparse.Namespace) -> None:
    require(os.geteuid() == 0, "root_shell_required")
    require(
        run("git", "branch", "--show-current").stdout.strip()
        == arguments.expected_branch,
        "unexpected_branch",
    )
    require(
        run("git", "rev-parse", "HEAD").stdout.strip()
        == arguments.expected_head,
        "unexpected_head",
    )
    require(
        run(
            "git",
            "status",
            "--porcelain",
            "--untracked-files=all",
        ).stdout.strip()
        == "",
        "worktree_not_clean",
    )
    require(arguments.candidate_daemon.is_file(), "candidate_daemon_missing")
    require(arguments.installed_daemon.is_file(), "installed_daemon_missing")
    require(arguments.loader.is_file(), "loader_missing")
    require(arguments.production_database.is_file(), "production_database_missing")
    require(
        sha256(arguments.candidate_daemon)
        == arguments.expected_daemon_sha256,
        "candidate_daemon_hash_mismatch",
    )
    require(
        sha256(arguments.loader) == arguments.expected_loader_sha256,
        "loader_hash_mismatch",
    )
    require(
        run(
            "systemctl",
            "is-active",
            arguments.service,
            check=False,
        ).stdout.strip()
        == "active",
        "service_not_active",
    )
    require(service_pid(arguments.service) > 0, "service_pid_missing")
    require(
        not arguments.evidence_dir.exists(),
        "evidence_directory_already_exists",
    )
    require(
        not override_path(arguments.service).exists(),
        "runtime_override_already_exists",
    )


def create_evidence(
    arguments: argparse.Namespace,
) -> tuple[Path, str, list[Path]]:
    arguments.evidence_dir.mkdir(mode=0o700, parents=True)
    backup_files: list[Path] = []

    daemon_backup = arguments.evidence_dir / "vdr-suite-daemon.before"
    shutil.copy2(arguments.installed_daemon, daemon_backup)
    backup_files.append(daemon_backup)

    loader_backup = arguments.evidence_dir / "deferred-runtime-loader.js.before"
    shutil.copy2(arguments.loader, loader_backup)
    backup_files.append(loader_backup)

    configuration_backup = arguments.evidence_dir / "vdr-suite-daemon.default.before"
    if copy_optional(arguments.configuration, configuration_backup):
        backup_files.append(configuration_backup)

    database_backup = arguments.evidence_dir / "production-database.before.sqlite"
    backup_database(arguments.production_database, database_backup)
    backup_files.append(database_backup)

    candidate_backup = arguments.evidence_dir / "vdr-suite-daemon.candidate"
    shutil.copy2(arguments.candidate_daemon, candidate_backup)
    backup_files.append(candidate_backup)

    production_fingerprint = database_logical_sha256(
        arguments.production_database
    )
    (arguments.evidence_dir / "production-database.logical-sha256.before").write_text(
        production_fingerprint + "\n",
        encoding="utf-8",
    )
    backup_files.append(
        arguments.evidence_dir / "production-database.logical-sha256.before"
    )

    write_checksums(arguments.evidence_dir, backup_files)
    return daemon_backup, production_fingerprint, backup_files


def run_inner(
    arguments: argparse.Namespace,
    scenario_database: Path,
) -> subprocess.CompletedProcess[str]:
    command = [
        sys.executable,
        str(INNER_RUNNER),
        "--run",
        "--base-url",
        arguments.base_url,
        "--database",
        str(scenario_database),
        "--service",
        arguments.service,
        "--daemon",
        str(arguments.installed_daemon),
        "--loader",
        str(arguments.loader),
        "--backup-dir",
        str(arguments.evidence_dir),
        "--expected-branch",
        arguments.expected_branch,
        "--expected-head",
        arguments.expected_head,
        "--expected-daemon-sha256",
        arguments.expected_daemon_sha256,
        "--expected-loader-sha256",
        arguments.expected_loader_sha256,
        "--report-json",
        str(arguments.report_json),
    ]
    return run(*command, check=False)


def verify_scenario_process(
    arguments: argparse.Namespace,
    scenario_database: Path,
) -> int:
    pid = wait_service(arguments.service)
    require(
        sha256(Path(f"/proc/{pid}/exe"))
        == arguments.expected_daemon_sha256,
        "running_candidate_daemon_hash_mismatch",
    )
    environment = process_environment(pid)
    expected = {
        f"VDR_SUITE_DATABASE_PATH={scenario_database}".encode(),
        f"VDR_SUITE_SECURITY_DATABASE_PATH={scenario_database}".encode(),
    }
    require(expected.issubset(environment), "scenario_database_override_missing")
    return pid


def execute(arguments: argparse.Namespace) -> dict[str, Any]:
    preflight(arguments)
    original_pid = service_pid(arguments.service)
    stop_service(arguments.service)

    daemon_backup: Path | None = None
    production_fingerprint = ""
    override = override_path(arguments.service)
    inner_result: subprocess.CompletedProcess[str] | None = None
    scenario_pid = 0
    accepted = False
    failure = ""

    try:
        daemon_backup, production_fingerprint, _ = create_evidence(arguments)
        scenario_database = arguments.evidence_dir / "slice2x-scenario.sqlite"
        backup_database(arguments.production_database, scenario_database)

        atomic_install(arguments.candidate_daemon, arguments.installed_daemon)
        write_override(override, scenario_database.resolve())
        run("systemctl", "daemon-reload")
        run("systemctl", "start", arguments.service)
        scenario_pid = verify_scenario_process(
            arguments,
            scenario_database.resolve(),
        )

        inner_result = run_inner(arguments, scenario_database.resolve())
        accepted = inner_result.returncode == 0
        if not accepted:
            failure = "inner_runtime_acceptance_failed"
    except Exception as error:
        failure = f"{type(error).__name__}:{error}"
    finally:
        try:
            stop_service(arguments.service)
        except Exception as error:
            failure = failure or f"scenario_stop_failed:{type(error).__name__}"

        try:
            remove_override(override)
            run("systemctl", "daemon-reload")
        except Exception as error:
            failure = failure or f"override_cleanup_failed:{type(error).__name__}"

        if not accepted and daemon_backup is not None:
            try:
                restore_exact(daemon_backup, arguments.installed_daemon)
            except Exception as error:
                failure = failure or f"daemon_restore_failed:{type(error).__name__}"

        try:
            run("systemctl", "start", arguments.service)
            final_pid = wait_service(arguments.service)
        except Exception as error:
            final_pid = 0
            failure = failure or f"production_restart_failed:{type(error).__name__}"

    final_daemon_sha256 = (
        sha256(arguments.installed_daemon)
        if arguments.installed_daemon.is_file()
        else "missing"
    )
    final_loader_sha256 = (
        sha256(arguments.loader) if arguments.loader.is_file() else "missing"
    )
    final_configuration_sha256 = (
        sha256(arguments.configuration)
        if arguments.configuration.is_file()
        else "missing"
    )
    final_database_fingerprint = database_logical_sha256(
        arguments.production_database
    )
    production_unchanged = (
        bool(production_fingerprint)
        and final_database_fingerprint == production_fingerprint
    )
    expected_final_daemon = (
        arguments.expected_daemon_sha256
        if accepted
        else sha256(daemon_backup) if daemon_backup is not None else "missing"
    )

    if final_daemon_sha256 != expected_final_daemon:
        failure = failure or "final_daemon_hash_mismatch"
    if final_loader_sha256 != arguments.expected_loader_sha256:
        failure = failure or "final_loader_hash_mismatch"
    if not production_unchanged:
        failure = failure or "production_database_changed"
    if override.exists() or override.is_symlink():
        failure = failure or "runtime_override_remains"
    if final_pid <= 0:
        failure = failure or "final_service_not_running"

    accepted = accepted and not failure
    outer_report = {
        "schemaVersion": 1,
        "passed": accepted,
        "head": arguments.expected_head,
        "originalServicePid": original_pid,
        "scenarioServicePid": scenario_pid,
        "finalServicePid": final_pid,
        "candidateDaemonSha256": arguments.expected_daemon_sha256,
        "finalDaemonSha256": final_daemon_sha256,
        "loaderSha256": final_loader_sha256,
        "configurationSha256": final_configuration_sha256,
        "productionDatabaseLogicalSha256Before": production_fingerprint,
        "productionDatabaseLogicalSha256After": final_database_fingerprint,
        "productionDatabaseUnchanged": production_unchanged,
        "runtimeOverrideRemoved": not override.exists(),
        "innerReturnCode": (
            inner_result.returncode if inner_result is not None else -1
        ),
        "innerStdout": (
            inner_result.stdout if inner_result is not None else ""
        ),
        "innerStderr": (
            inner_result.stderr if inner_result is not None else ""
        ),
        "error": failure,
    }
    outer_path = arguments.evidence_dir / "slice2x-runtime-acceptance.json"
    outer_path.write_text(
        json.dumps(outer_report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    checksum_files = [
        path
        for path in arguments.evidence_dir.iterdir()
        if path.is_file() and path.name != "SHA256SUMS"
    ]
    write_checksums(arguments.evidence_dir, checksum_files)
    return outer_report


def self_test() -> None:
    with tempfile.TemporaryDirectory(
        prefix="phase62-slice2x-entry-"
    ) as directory:
        root = Path(directory)
        database = root / "database.sqlite"
        connection = sqlite3.connect(database)
        connection.execute("CREATE TABLE sample (id INTEGER PRIMARY KEY, value TEXT)")
        connection.execute("INSERT INTO sample (value) VALUES ('ok')")
        connection.commit()
        connection.close()

        fingerprint = database_logical_sha256(database)
        backup = root / "backup.sqlite"
        backup_database(database, backup)
        require(
            database_logical_sha256(backup) == fingerprint,
            "database_backup_self_test_failed",
        )

        rendered = render_override(backup.resolve())
        require(
            "VDR_SUITE_DATABASE_PATH=" in rendered
            and "VDR_SUITE_SECURITY_DATABASE_PATH=" in rendered,
            "override_render_self_test_failed",
        )

        evidence = root / "evidence"
        evidence.mkdir()
        sample = evidence / "sample"
        sample.write_text("ok\n", encoding="utf-8")
        write_checksums(evidence, [sample])
        require(
            (evidence / "SHA256SUMS").read_text(encoding="utf-8")
            == f"{sha256(sample)}  sample\n",
            "checksum_self_test_failed",
        )

    print("PHASE_62_SLICE_2X_RUNTIME_ENTRY_SELF_TEST=PASS")


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Install and validate Slice 2X against an isolated yaVDR scenario database."
        )
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--run", action="store_true")
    parser.add_argument("--service", default=DEFAULT_SERVICE)
    parser.add_argument(
        "--candidate-daemon",
        type=Path,
        default=DEFAULT_CANDIDATE_DAEMON,
    )
    parser.add_argument(
        "--installed-daemon",
        type=Path,
        default=DEFAULT_INSTALLED_DAEMON,
    )
    parser.add_argument("--loader", type=Path, default=DEFAULT_LOADER)
    parser.add_argument(
        "--configuration",
        type=Path,
        default=DEFAULT_CONFIGURATION,
    )
    parser.add_argument(
        "--production-database",
        type=Path,
        default=DEFAULT_DATABASE,
    )
    parser.add_argument("--evidence-dir", type=Path, default=Path())
    parser.add_argument("--report-json", type=Path, default=Path())
    parser.add_argument("--base-url", default="http://127.0.0.1:18080")
    parser.add_argument("--expected-branch", default="")
    parser.add_argument("--expected-head", default="")
    parser.add_argument("--expected-daemon-sha256", default="")
    parser.add_argument("--expected-loader-sha256", default="")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.self_test:
        self_test()
        return 0

    for name in (
        "expected_branch",
        "expected_head",
        "expected_daemon_sha256",
        "expected_loader_sha256",
    ):
        require(bool(getattr(arguments, name)), f"missing_argument:{name}")
    require(arguments.evidence_dir != Path(), "evidence_directory_required")
    require(arguments.evidence_dir.is_absolute(), "evidence_directory_must_be_absolute")
    if arguments.report_json == Path():
        arguments.report_json = (
            arguments.evidence_dir
            / "protected-mutation-outcome-runtime-acceptance.json"
        )

    report = execute(arguments)
    if not report["passed"]:
        print("PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={report['error']}")
        return 1

    print("PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=PASS")
    print(f"HEAD={report['head']}")
    print(f"DAEMON_SHA256={report['finalDaemonSha256']}")
    print(f"LOADER_SHA256={report['loaderSha256']}")
    print(f"CONFIGURATION_SHA256={report['configurationSha256']}")
    print(
        "PRODUCTION_DATABASE_LOGICAL_SHA256="
        f"{report['productionDatabaseLogicalSha256After']}"
    )
    print(f"EVIDENCE={arguments.evidence_dir}")
    print(f"FINAL_SERVICE_PID={report['finalServicePid']}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print("PHASE_62_SLICE_2X_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        raise SystemExit(1)
