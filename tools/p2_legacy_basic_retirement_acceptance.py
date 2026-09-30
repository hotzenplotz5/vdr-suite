#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import getpass
import hashlib
import http.client
import json
import os
import shutil
import sqlite3
import stat
import subprocess
import sys
import tempfile
import time
import warnings
from contextlib import closing
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

warnings.filterwarnings("ignore", category=DeprecationWarning)
try:
    import crypt
except ImportError:
    crypt = None

DEFAULT_SERVICE = "vdr-suite-daemon.service"
DEFAULT_DAEMON = "/usr/sbin/vdr-suite-daemon"
DEFAULT_CONFIGURATION = "/etc/default/vdr-suite-daemon"
DEFAULT_DATABASE = "/var/lib/vdr-suite/vdr-suite.db"
DEFAULT_BUILT_DAEMON = ".build/vdr-suite-daemon"
DEFAULT_BACKUP_ROOT = "/var/backups"
DEFAULT_HTTP_PORT = 18080
DEFAULT_LEGACY_AUTHORIZATION = "Basic YWRtaW46dmRyLXN1aXRl"
SECURITY_MODE_KEY = "VDR_SUITE_SECURITY_MODE"


class AcceptanceError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AcceptanceError(message)


def run(root: Path, *arguments: str, check: bool = True) -> str:
    completed = subprocess.run(
        arguments,
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and completed.returncode != 0:
        command = arguments[0] if arguments else "unknown"
        raise AcceptanceError(f"command_failed:{command}")
    return completed.stdout.strip()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def configuration_sha(path: Path) -> str:
    if not path.exists():
        return "absent"
    require(path.is_file() and not path.is_symlink(), "configuration_not_regular_file")
    return sha256(path)


def parse_env_file(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    require(path.is_file() and not path.is_symlink(), "configuration_not_regular_file")
    values: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        value = value.strip()
        if (
            len(value) >= 2
            and value[0] == value[-1]
            and value[0] in ("'", '"')
        ):
            value = value[1:-1]
        values[key.strip()] = value
    return values


def render_security_mode(original: str, mode: str) -> str:
    require(mode in ("legacy-basic", "enforced"), "invalid_security_mode")
    kept = [
        line
        for line in original.splitlines()
        if not line.lstrip().startswith(SECURITY_MODE_KEY + "=")
    ]
    content = "\n".join(kept)
    if content:
        content += "\n"
    content += f"{SECURITY_MODE_KEY}={mode}\n"
    return content


def atomic_write_security_mode(path: Path, mode: str) -> None:
    require(not path.is_symlink(), "configuration_symlink_not_supported")
    original = path.read_text(encoding="utf-8") if path.exists() else ""
    content = render_security_mode(original, mode)
    temporary = path.with_name(path.name + ".p2-retirement-new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "configuration_temporary_exists",
    )

    if path.exists():
        metadata = path.stat()
        file_mode = stat.S_IMODE(metadata.st_mode)
        uid = metadata.st_uid
        gid = metadata.st_gid
    else:
        path.parent.mkdir(mode=0o755, parents=True, exist_ok=True)
        file_mode = 0o644
        uid = 0
        gid = 0

    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL
    if hasattr(os, "O_NOFOLLOW"):
        flags |= os.O_NOFOLLOW
    descriptor = os.open(temporary, flags, file_mode)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8") as handle:
            descriptor = -1
            handle.write(content)
            handle.flush()
            os.fsync(handle.fileno())
        os.chmod(temporary, file_mode)
        os.chown(temporary, uid, gid)
        os.replace(temporary, path)
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        if temporary.exists() or temporary.is_symlink():
            temporary.unlink()


def restore_configuration(
    backup: Path,
    destination: Path,
    initially_present: bool,
) -> None:
    if not initially_present:
        if destination.exists() or destination.is_symlink():
            require(not destination.is_dir(), "configuration_restore_target_is_directory")
            destination.unlink()
        return

    require(backup.is_file(), "configuration_backup_missing")
    temporary = destination.with_name(destination.name + ".p2-retirement-restore")
    shutil.copy2(backup, temporary)
    os.replace(temporary, destination)


def atomic_install_binary(source: Path, destination: Path) -> None:
    require(source.is_file(), "candidate_daemon_missing")
    require(destination.is_file(), "installed_daemon_missing")
    require(not destination.is_symlink(), "installed_daemon_symlink_not_supported")
    metadata = destination.stat()
    temporary = destination.with_name(destination.name + ".p2-retirement-new")
    require(
        not temporary.exists() and not temporary.is_symlink(),
        "daemon_temporary_exists",
    )
    shutil.copyfile(source, temporary)
    os.chmod(temporary, stat.S_IMODE(metadata.st_mode))
    os.chown(temporary, metadata.st_uid, metadata.st_gid)
    os.replace(temporary, destination)


def restore_binary(backup: Path, destination: Path) -> None:
    require(backup.is_file(), "daemon_backup_missing")
    metadata = backup.stat()
    temporary = destination.with_name(destination.name + ".p2-retirement-restore")
    shutil.copyfile(backup, temporary)
    os.chmod(temporary, stat.S_IMODE(metadata.st_mode))
    os.chown(temporary, metadata.st_uid, metadata.st_gid)
    os.replace(temporary, destination)


def service_pid(root: Path, service: str) -> int:
    value = run(
        root,
        "systemctl",
        "show",
        "-p",
        "MainPID",
        "--value",
        service,
    )
    return int(value or "0")


def wait_service(root: Path, service: str, timeout: float = 25.0) -> int:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        active = run(
            root,
            "systemctl",
            "is-active",
            service,
            check=False,
        )
        if active == "active":
            pid = service_pid(root, service)
            if pid > 0:
                return pid
        time.sleep(0.25)
    raise AcceptanceError("service_did_not_become_active")


def request(
    port: int,
    method: str,
    path: str,
    request_id: str,
    *,
    authorization: str = "",
    cookie: str = "",
    csrf: str = "",
) -> tuple[int, dict[str, str], str]:
    headers = {
        "Accept": "application/json",
        "Connection": "close",
        "X-Request-ID": request_id,
    }
    if authorization:
        headers["Authorization"] = authorization
    if cookie:
        headers["Cookie"] = "vdr_suite_session=" + cookie
    if csrf:
        headers["X-CSRF-Token"] = csrf

    connection = http.client.HTTPConnection(
        "127.0.0.1",
        port,
        timeout=10,
    )
    try:
        connection.request(method, path, body=None, headers=headers)
        response = connection.getresponse()
        body = response.read().decode("utf-8", errors="replace")
        response_headers = {
            key.lower(): value
            for key, value in response.getheaders()
        }
        return response.status, response_headers, body
    finally:
        connection.close()


def wait_http(port: int, timeout: float = 25.0) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            status, _, _ = request(
                port,
                "GET",
                "/api/backends",
                "p2-retirement-readiness",
            )
            if 100 <= status <= 599:
                return
        except OSError:
            pass
        time.sleep(0.25)
    raise AcceptanceError("http_endpoint_did_not_become_ready")


def basic_authorization(login: str, password: str) -> str:
    encoded = base64.b64encode(
        f"{login}:{password}".encode("utf-8")
    ).decode("ascii")
    return "Basic " + encoded


def decode_basic_authorization(authorization: str) -> tuple[str, str]:
    require(
        authorization.startswith("Basic "),
        "legacy_authorization_not_basic",
    )
    try:
        decoded = base64.b64decode(
            authorization[6:],
            validate=True,
        ).decode("utf-8")
    except Exception as error:
        raise AcceptanceError("legacy_authorization_invalid") from error
    separator = decoded.find(":")
    require(separator >= 0, "legacy_authorization_invalid")
    return decoded[:separator], decoded[separator + 1:]


def parse_session_response(
    headers: dict[str, str],
    body: str,
) -> tuple[str, str]:
    set_cookie = headers.get("set-cookie", "")
    prefix = "vdr_suite_session="
    start = set_cookie.find(prefix)
    require(start >= 0, "browser_session_cookie_missing")
    value_start = start + len(prefix)
    value_end = set_cookie.find(";", value_start)
    cookie = set_cookie[
        value_start:
        None if value_end < 0 else value_end
    ]
    require(cookie and "." in cookie, "browser_session_cookie_invalid")

    try:
        decoded = json.loads(body)
    except json.JSONDecodeError as error:
        raise AcceptanceError("browser_session_response_invalid_json") from error
    require(isinstance(decoded, dict), "browser_session_response_not_object")
    csrf = decoded.get("csrfToken")
    require(isinstance(csrf, str) and csrf, "browser_session_csrf_missing")
    return cookie, csrf


def human_session_roundtrip(
    port: int,
    login: str,
    password: str,
    label: str,
) -> tuple[int, int, int]:
    authorization = basic_authorization(login, password)
    status, headers, body = request(
        port,
        "POST",
        "/api/security/browser-sessions",
        f"{label}-login",
        authorization=authorization,
    )
    require(status == 200, f"{label}_human_login_status_{status}")
    cookie, csrf = parse_session_response(headers, body)

    read_status, _, _ = request(
        port,
        "GET",
        "/api/backends",
        f"{label}-read",
        cookie=cookie,
    )
    require(read_status == 200, f"{label}_human_read_status_{read_status}")

    logout_status, _, _ = request(
        port,
        "POST",
        "/api/security/browser-sessions/logout",
        f"{label}-logout",
        cookie=cookie,
        csrf=csrf,
    )
    require(logout_status == 204, f"{label}_human_logout_status_{logout_status}")
    return status, read_status, logout_status


def legacy_probe(
    port: int,
    authorization: str,
    expected_status: int,
    label: str,
) -> int:
    status, _, _ = request(
        port,
        "GET",
        "/api/backends",
        f"{label}-legacy",
        authorization=authorization,
    )
    require(status == expected_status, f"{label}_legacy_status_{status}")
    return status


def database_connection(path: Path) -> sqlite3.Connection:
    database = sqlite3.connect(
        f"file:{path}?mode=ro",
        uri=True,
        timeout=10,
    )
    database.execute("PRAGMA busy_timeout=10000")
    database.execute("PRAGMA foreign_keys=ON")
    return database


def verify_database(database: sqlite3.Connection) -> tuple[str, int]:
    quick = str(database.execute("PRAGMA quick_check").fetchone()[0])
    foreign_keys = database.execute("PRAGMA foreign_key_check").fetchall()
    require(quick == "ok", "sqlite_quick_check_failed")
    require(not foreign_keys, "sqlite_foreign_key_check_failed")
    return quick, len(foreign_keys)


def eligible_human_admins(
    database: sqlite3.Connection,
    requested_login: str,
) -> list[tuple[str, str, str, str, str]]:
    parameters: tuple[Any, ...] = ()
    login_filter = ""
    if requested_login:
        login_filter = "AND verifier.login_name = ? "
        parameters = (requested_login,)

    rows = database.execute(
        """
        SELECT
            account.account_id,
            account.actor_id,
            verifier.login_name,
            credential.credential_id,
            verifier.password_hash
        FROM security_human_accounts AS account
        JOIN security_actors AS actor
          ON actor.actor_id = account.actor_id
        JOIN security_credentials AS credential
          ON credential.actor_id = actor.actor_id
        JOIN security_basic_credential_verifiers AS verifier
          ON verifier.credential_id = credential.credential_id
        JOIN security_actor_permission_grants AS grant_record
          ON grant_record.actor_id = actor.actor_id
        WHERE account.active <> 0
          AND actor.actor_type = 'user'
          AND actor.active <> 0
          AND actor.revoked_at = ''
          AND credential.credential_type = 'human-password'
          AND credential.active <> 0
          AND credential.revoked_at = ''
          AND (credential.expires_at = '' OR credential.expires_at > CURRENT_TIMESTAMP)
          AND grant_record.active <> 0
          AND grant_record.revoked_at = ''
          AND grant_record.backend_id = '*'
          AND (grant_record.permission = 'role.admin'
               OR grant_record.permission = '*')
        """
        + login_filter
        + """
        GROUP BY
            account.account_id,
            account.actor_id,
            verifier.login_name,
            credential.credential_id,
            verifier.password_hash
        ORDER BY verifier.login_name, account.account_id
        """,
        parameters,
    ).fetchall()
    return [
        tuple(str(value) for value in row)
        for row in rows
    ]


def verify_human_password(password: str, password_hash: str) -> None:
    require(crypt is not None, "python_crypt_module_unavailable")
    require(
        password_hash.startswith("$y$") or password_hash.startswith("$6$"),
        "unsupported_human_password_hash",
    )
    verified = crypt.crypt(password, password_hash)
    require(
        isinstance(verified, str) and verified == password_hash,
        "human_password_preflight_failed",
    )


def identity_fingerprint(
    database: sqlite3.Connection,
    account_id: str,
    actor_id: str,
    login_name: str,
    credential_id: str,
) -> str:
    payload: dict[str, Any] = {
        "account": database.execute(
            """
            SELECT account_id, actor_id, display_name, active
            FROM security_human_accounts
            WHERE account_id = ?
            """,
            (account_id,),
        ).fetchone(),
        "actor": database.execute(
            """
            SELECT actor_id, actor_type, display_name, active, revoked_at
            FROM security_actors
            WHERE actor_id = ?
            """,
            (actor_id,),
        ).fetchone(),
        "credential": database.execute(
            """
            SELECT credential_id, actor_id, credential_type, active,
                   expires_at, revoked_at, rotated_from_credential_id
            FROM security_credentials
            WHERE credential_id = ?
            """,
            (credential_id,),
        ).fetchone(),
        "verifier": database.execute(
            """
            SELECT credential_id, login_name, password_hash
            FROM security_basic_credential_verifiers
            WHERE credential_id = ? AND login_name = ?
            """,
            (credential_id, login_name),
        ).fetchone(),
        "grants": database.execute(
            """
            SELECT permission, backend_id, active, revoked_at
            FROM security_actor_permission_grants
            WHERE actor_id = ?
            ORDER BY permission, backend_id
            """,
            (actor_id,),
        ).fetchall(),
    }
    require(all(payload[key] is not None for key in ("account", "actor", "credential", "verifier")),
            "identity_fingerprint_source_missing")
    canonical = json.dumps(
        payload,
        sort_keys=True,
        separators=(",", ":"),
        default=list,
    ).encode("utf-8")
    return hashlib.sha256(canonical).hexdigest()


def runtime_environment(pid: int) -> set[bytes]:
    return set(Path(f"/proc/{pid}/environ").read_bytes().split(b"\0"))


def verify_runtime_mode(pid: int, mode: str) -> None:
    expected = f"{SECURITY_MODE_KEY}={mode}".encode("utf-8")
    require(expected in runtime_environment(pid), "runtime_security_mode_mismatch")


def write_report(path: Path, values: list[tuple[str, object]]) -> None:
    path.write_text(
        "".join(f"{key}={value}\n" for key, value in values),
        encoding="utf-8",
    )
    os.chmod(path, 0o600)


def self_test() -> int:
    sample = (
        "# existing deployment\n"
        "VDR_SUITE_BASIC_AUTH='Basic stale-value'\n"
        "VDR_SUITE_SECURITY_MODE=legacy-basic\n"
        "VDR_SUITE_UPGRADE_SENTINEL=preserve\n"
    )
    enforced = render_security_mode(sample, "enforced")
    require(
        enforced.count("VDR_SUITE_SECURITY_MODE=") == 1,
        "self_test_duplicate_enforced_mode",
    )
    require(
        "VDR_SUITE_SECURITY_MODE=enforced\n" in enforced,
        "self_test_enforced_mode_missing",
    )
    require(
        "VDR_SUITE_BASIC_AUTH='Basic stale-value'" in enforced,
        "self_test_stale_legacy_input_not_preserved",
    )
    require(
        "VDR_SUITE_UPGRADE_SENTINEL=preserve" in enforced,
        "self_test_sentinel_not_preserved",
    )

    rolled_back = render_security_mode(enforced, "legacy-basic")
    require(
        rolled_back.count("VDR_SUITE_SECURITY_MODE=") == 1,
        "self_test_duplicate_rollback_mode",
    )
    require(
        "VDR_SUITE_SECURITY_MODE=legacy-basic\n" in rolled_back,
        "self_test_rollback_mode_missing",
    )

    with tempfile.TemporaryDirectory(prefix="vdr-suite-p2-retirement-") as directory:
        path = Path(directory) / "defaults"
        path.write_text(enforced, encoding="utf-8")
        values = parse_env_file(path)
        require(values[SECURITY_MODE_KEY] == "enforced", "self_test_parse_mode")
        require(
            values["VDR_SUITE_BASIC_AUTH"] == "Basic stale-value",
            "self_test_parse_legacy_authorization",
        )

    print("P2_LEGACY_BASIC_RETIREMENT_ACCEPTANCE_SELF_TEST=PASS")
    return 0


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Guarded real yaVDR Legacy Basic retirement migration acceptance"
    )
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--repository", default="/home/yavdr/vdr-suite")
    parser.add_argument("--expected-remote-ref")
    parser.add_argument("--expected-head")
    parser.add_argument("--expected-installed-daemon-sha256")
    parser.add_argument("--expected-candidate-daemon-sha256")
    parser.add_argument("--expected-configuration-sha256")
    parser.add_argument("--expected-service-pid", type=int)
    parser.add_argument("--source-ci-run", type=int)
    parser.add_argument("--source-ci-run-id", type=int)
    parser.add_argument("--human-login", default="")
    parser.add_argument("--backup-root", default=DEFAULT_BACKUP_ROOT)
    parser.add_argument("--service", default=DEFAULT_SERVICE)
    parser.add_argument("--daemon", default=DEFAULT_DAEMON)
    parser.add_argument("--built-daemon", default=DEFAULT_BUILT_DAEMON)
    parser.add_argument("--configuration", default=DEFAULT_CONFIGURATION)
    parser.add_argument("--database", default=DEFAULT_DATABASE)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.self_test:
        require(not arguments.run, "self_test_and_run_are_mutually_exclusive")
        return self_test()

    require(arguments.run, "explicit_run_flag_required")
    require(os.geteuid() == 0, "root_required")

    for name in (
        "expected_remote_ref",
        "expected_head",
        "expected_installed_daemon_sha256",
        "expected_candidate_daemon_sha256",
        "expected_configuration_sha256",
        "expected_service_pid",
        "source_ci_run",
        "source_ci_run_id",
    ):
        require(getattr(arguments, name) not in (None, ""), f"{name}_required")

    root = Path(arguments.repository).resolve()
    daemon = Path(arguments.daemon)
    built_daemon = (root / arguments.built_daemon).resolve()
    configuration = Path(arguments.configuration)
    database_path = Path(arguments.database)

    require(Path.cwd().resolve() == root, "unexpected_working_directory")
    require(
        run(root, "git", "rev-parse", "HEAD") == arguments.expected_head,
        "unexpected_local_head",
    )
    require(
        run(root, "git", "rev-parse", arguments.expected_remote_ref)
        == arguments.expected_head,
        "unexpected_remote_ref",
    )
    require(run(root, "git", "status", "--porcelain") == "", "worktree_not_clean")

    require(built_daemon.is_file(), "candidate_daemon_missing")
    require(daemon.is_file() and not daemon.is_symlink(), "installed_daemon_missing")
    require(database_path.is_file(), "database_missing")
    require(
        sha256(built_daemon) == arguments.expected_candidate_daemon_sha256,
        "candidate_daemon_fingerprint_changed",
    )
    require(
        sha256(daemon) == arguments.expected_installed_daemon_sha256,
        "installed_daemon_fingerprint_changed",
    )
    require(
        configuration_sha(configuration) == arguments.expected_configuration_sha256,
        "configuration_fingerprint_changed",
    )
    require(
        run(root, "systemctl", "is-active", arguments.service) == "active",
        "service_not_active",
    )
    initial_pid = service_pid(root, arguments.service)
    require(initial_pid == arguments.expected_service_pid, "service_pid_changed")
    require(
        sha256(Path(f"/proc/{initial_pid}/exe"))
        == arguments.expected_installed_daemon_sha256,
        "running_daemon_fingerprint_changed",
    )

    initial_configuration_present = configuration.exists()
    if initial_configuration_present:
        require(
            configuration.is_file() and not configuration.is_symlink(),
            "configuration_not_regular_file",
        )
    initial_values = parse_env_file(configuration)
    configured_mode = initial_values.get(SECURITY_MODE_KEY, "")
    require(
        configured_mode in ("", "legacy-basic"),
        "existing_deployment_not_in_legacy_basic_mode",
    )

    legacy_authorization = initial_values.get(
        "VDR_SUITE_BASIC_AUTH",
        DEFAULT_LEGACY_AUTHORIZATION,
    )
    if not legacy_authorization:
        legacy_authorization = DEFAULT_LEGACY_AUTHORIZATION
    legacy_login, legacy_password = decode_basic_authorization(
        legacy_authorization
    )
    managed_login = initial_values.get("VDR_SUITE_MANAGED_BASIC_USERNAME", "")
    managed_hash = initial_values.get("VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH", "")
    if managed_login and managed_login == legacy_login:
        require(
            crypt is not None and bool(managed_hash),
            "legacy_probe_ambiguous_with_managed_basic",
        )
        managed_match = crypt.crypt(legacy_password, managed_hash)
        require(
            managed_match != managed_hash,
            "legacy_probe_ambiguous_with_managed_basic",
        )
    legacy_password = ""

    port_text = initial_values.get("VDR_SUITE_HTTP_PORT", str(DEFAULT_HTTP_PORT))
    require(port_text.isdigit(), "invalid_http_port")
    port = int(port_text)
    require(1 <= port <= 65535, "invalid_http_port")

    with closing(database_connection(database_path)) as database:
        initial_quick, initial_foreign_keys = verify_database(database)
        admins = eligible_human_admins(database, arguments.human_login)
        require(len(admins) == 1, f"eligible_human_admin_count_{len(admins)}")
        account_id, actor_id, login_name, credential_id, password_hash = admins[0]
        identity_before = identity_fingerprint(
            database,
            account_id,
            actor_id,
            login_name,
            credential_id,
        )

    password = getpass.getpass(
        f"Human Account password for {login_name}: "
    )
    require(bool(password), "human_password_required")
    verify_human_password(password, password_hash)
    password_hash = ""

    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    evidence = Path(arguments.backup_root) / (
        "vdr-suite-legacy-basic-retirement-"
        f"{timestamp}-{arguments.expected_head[:12]}"
    )
    evidence.mkdir(mode=0o700, parents=False, exist_ok=False)

    daemon_backup = evidence / "vdr-suite-daemon.before"
    configuration_backup = evidence / "vdr-suite-daemon.default.before"
    report_path = evidence / "runtime-acceptance-report.txt"

    shutil.copy2(daemon, daemon_backup)
    if initial_configuration_present:
        shutil.copy2(configuration, configuration_backup)

    backup_checksums = evidence / "SHA256SUMS"
    checksum_lines = [f"{sha256(daemon_backup)}  {daemon_backup.name}\n"]
    if initial_configuration_present:
        checksum_lines.append(
            f"{sha256(configuration_backup)}  {configuration_backup.name}\n"
        )
    backup_checksums.write_text("".join(checksum_lines), encoding="utf-8")
    os.chmod(backup_checksums, 0o600)

    success = False
    failure_reason = ""
    restoration_status = "not-needed"
    baseline_legacy_status = 0
    baseline_human_login_status = 0
    enforced_legacy_status = 0
    enforced_human_login_status = 0
    rollback_legacy_status = 0
    rollback_human_login_status = 0
    final_legacy_status = 0
    final_human_login_status = 0
    final_pid = 0
    identity_after = ""
    final_quick = ""
    final_foreign_keys = -1

    try:
        run(root, "systemctl", "stop", arguments.service)
        require(
            run(
                root,
                "systemctl",
                "is-active",
                arguments.service,
                check=False,
            ) in ("inactive", "failed"),
            "service_did_not_stop",
        )

        atomic_install_binary(built_daemon, daemon)
        require(
            sha256(daemon) == arguments.expected_candidate_daemon_sha256,
            "installed_candidate_daemon_mismatch",
        )

        run(root, "systemctl", "start", arguments.service)
        baseline_pid = wait_service(root, arguments.service)
        require(
            sha256(Path(f"/proc/{baseline_pid}/exe"))
            == arguments.expected_candidate_daemon_sha256,
            "running_candidate_daemon_mismatch",
        )
        wait_http(port)

        baseline_legacy_status = legacy_probe(
            port,
            legacy_authorization,
            200,
            "baseline",
        )
        baseline_human_login_status, _, _ = human_session_roundtrip(
            port,
            login_name,
            password,
            "baseline",
        )

        atomic_write_security_mode(configuration, "enforced")
        run(root, "systemctl", "restart", arguments.service)
        enforced_pid = wait_service(root, arguments.service)
        verify_runtime_mode(enforced_pid, "enforced")
        wait_http(port)

        enforced_legacy_status = legacy_probe(
            port,
            legacy_authorization,
            401,
            "enforced",
        )
        enforced_human_login_status, _, _ = human_session_roundtrip(
            port,
            login_name,
            password,
            "enforced",
        )

        atomic_write_security_mode(configuration, "legacy-basic")
        run(root, "systemctl", "restart", arguments.service)
        rollback_pid = wait_service(root, arguments.service)
        verify_runtime_mode(rollback_pid, "legacy-basic")
        wait_http(port)

        rollback_legacy_status = legacy_probe(
            port,
            legacy_authorization,
            200,
            "rollback",
        )
        rollback_human_login_status, _, _ = human_session_roundtrip(
            port,
            login_name,
            password,
            "rollback",
        )

        atomic_write_security_mode(configuration, "enforced")
        run(root, "systemctl", "restart", arguments.service)
        final_pid = wait_service(root, arguments.service)
        verify_runtime_mode(final_pid, "enforced")
        wait_http(port)

        final_legacy_status = legacy_probe(
            port,
            legacy_authorization,
            401,
            "final-enforced",
        )
        final_human_login_status, _, _ = human_session_roundtrip(
            port,
            login_name,
            password,
            "final-enforced",
        )

        require(
            sha256(daemon) == arguments.expected_candidate_daemon_sha256,
            "final_installed_daemon_mismatch",
        )
        require(
            sha256(Path(f"/proc/{final_pid}/exe"))
            == arguments.expected_candidate_daemon_sha256,
            "final_running_daemon_mismatch",
        )
        require(
            parse_env_file(configuration).get(SECURITY_MODE_KEY) == "enforced",
            "final_configuration_not_enforced",
        )
        require(run(root, "git", "status", "--porcelain") == "", "worktree_changed")

        with closing(database_connection(database_path)) as database:
            final_quick, final_foreign_keys = verify_database(database)
            current_admins = eligible_human_admins(database, login_name)
            require(len(current_admins) == 1, "final_human_admin_resolution_changed")
            current_account_id, current_actor_id, current_login_name, current_credential_id, _ = current_admins[0]
            require(current_account_id == account_id, "final_account_id_changed")
            require(current_actor_id == actor_id, "final_actor_id_changed")
            require(current_login_name == login_name, "final_login_name_changed")
            require(current_credential_id == credential_id, "final_credential_id_changed")
            identity_after = identity_fingerprint(
                database,
                account_id,
                actor_id,
                login_name,
                credential_id,
            )

        require(identity_after == identity_before, "persistent_identity_changed")
        success = True

    except Exception as error:
        failure_reason = (
            str(error)
            if isinstance(error, AcceptanceError)
            else error.__class__.__name__
        )

    finally:
        password = ""

        def report_values(outcome: str) -> list[tuple[str, object]]:
            return [
                ("P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE", outcome),
                ("head", arguments.expected_head),
                ("source_ci_run", arguments.source_ci_run),
                ("source_ci_run_id", arguments.source_ci_run_id),
                ("candidate_daemon_sha256", arguments.expected_candidate_daemon_sha256),
                ("initial_installed_daemon_sha256", arguments.expected_installed_daemon_sha256),
                ("initial_configuration_sha256", arguments.expected_configuration_sha256),
                ("initial_effective_mode", configured_mode or "legacy-basic-fallback"),
                ("human_account_id", account_id),
                ("human_actor_id", actor_id),
                ("human_login", login_name),
                ("human_credential_id", credential_id),
                ("identity_fingerprint_before", identity_before),
                ("identity_fingerprint_after", identity_after),
                ("baseline_legacy_status", baseline_legacy_status),
                ("baseline_human_login_status", baseline_human_login_status),
                ("enforced_legacy_status", enforced_legacy_status),
                ("enforced_human_login_status", enforced_human_login_status),
                ("rollback_legacy_status", rollback_legacy_status),
                ("rollback_human_login_status", rollback_human_login_status),
                ("final_enforced_legacy_status", final_legacy_status),
                ("final_enforced_human_login_status", final_human_login_status),
                ("initial_sqlite_quick_check", initial_quick),
                ("initial_sqlite_foreign_key_violations", initial_foreign_keys),
                ("final_sqlite_quick_check", final_quick),
                ("final_sqlite_foreign_key_violations", final_foreign_keys),
                ("final_service_pid", final_pid),
                (
                    "final_configuration_mode",
                    parse_env_file(configuration).get(SECURITY_MODE_KEY, "")
                    if success else "",
                ),
                ("failure_restoration", restoration_status),
                ("failure_reason", failure_reason),
                ("evidence_directory", evidence),
            ]

        if success:
            try:
                write_report(report_path, report_values("PASS"))
                report_sha = sha256(report_path)
                report_checksum = evidence / "runtime-acceptance-report.sha256"
                report_checksum.write_text(
                    f"{report_sha}  {report_path.name}\n",
                    encoding="utf-8",
                )
                os.chmod(report_checksum, 0o600)
            except Exception:
                success = False
                failure_reason = "report_persistence_failed"

        if not success:
            restoration_errors: list[str] = []
            try:
                run(root, "systemctl", "stop", arguments.service, check=False)
            except Exception:
                restoration_errors.append("service_stop")
            try:
                restore_configuration(
                    configuration_backup,
                    configuration,
                    initial_configuration_present,
                )
            except Exception:
                restoration_errors.append("configuration")
            try:
                restore_binary(daemon_backup, daemon)
            except Exception:
                restoration_errors.append("daemon")
            try:
                run(root, "systemctl", "start", arguments.service, check=False)
                restored_pid = wait_service(root, arguments.service)
                require(
                    sha256(Path(f"/proc/{restored_pid}/exe"))
                    == arguments.expected_installed_daemon_sha256,
                    "restored_running_daemon_mismatch",
                )
                require(
                    configuration_sha(configuration)
                    == arguments.expected_configuration_sha256,
                    "restored_configuration_mismatch",
                )
            except Exception:
                restoration_errors.append("service_restore")
            restoration_status = (
                "pass"
                if not restoration_errors
                else "fail:" + ",".join(restoration_errors)
            )
            try:
                write_report(report_path, report_values("FAIL"))
                report_sha = sha256(report_path)
                report_checksum = evidence / "runtime-acceptance-report.sha256"
                report_checksum.write_text(
                    f"{report_sha}  {report_path.name}\n",
                    encoding="utf-8",
                )
                os.chmod(report_checksum, 0o600)
            except Exception:
                pass

    if not success:
        print("P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={failure_reason or 'unknown'}")
        print(f"FAILURE_RESTORATION={restoration_status}")
        print(f"EVIDENCE={evidence}")
        return 1

    print("P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=PASS")
    print(f"HEAD={arguments.expected_head}")
    print(f"CANDIDATE_DAEMON_SHA256={arguments.expected_candidate_daemon_sha256}")
    print("BASELINE_LEGACY_STATUS=200")
    print("ENFORCED_LEGACY_STATUS=401")
    print("ROLLBACK_LEGACY_STATUS=200")
    print("FINAL_ENFORCED_LEGACY_STATUS=401")
    print("HUMAN_ACCOUNT_LOGIN=PASS")
    print("PERSISTENT_IDENTITY_UNCHANGED=PASS")
    print("FINAL_SECURITY_MODE=enforced")
    print(f"EVIDENCE={evidence}")
    print(f"RUNTIME_REPORT_SHA256={sha256(report_path)}")
    print(f"FINAL_SERVICE_PID={final_pid}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AcceptanceError as error:
        print("P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=FAIL")
        print(f"FAILURE_REASON={error}")
        raise SystemExit(1)
