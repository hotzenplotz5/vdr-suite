#!/usr/bin/env python3
"""Conservative, persistent daemon-only candidate builder and installer.

CHECK never writes, BUILD never installs, APPLY never builds.  A DB backup is
never implicit.  Source provenance and schema risk must be known before APPLY.
"""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import shutil
import sqlite3
import stat
import subprocess
import sys
import tempfile
import time

SHA40 = re.compile(r"[0-9a-f]{40}\Z")
VERSION = re.compile(rb"git-([0-9a-f]{12})(?![0-9a-f])")
SCHEMA = re.compile(
    r"\b(?:ensureSchema|schemaVersion|schema_version|migrat\w*|"
    r"CREATE\s+(?:TABLE|INDEX|TRIGGER)|ALTER\s+TABLE|DROP\s+(?:TABLE|INDEX|TRIGGER)|"
    r"PRAGMA\s+user_version|DELETE\s+FROM)\b", re.IGNORECASE
)
FEATURES = (b"device_subtitle_delivery_invalid", b"track-status", b"/tracks", b"/subtitles")
BUILD_BUDGET = 2 * 1024 ** 3
MIN_HEADROOM = 4 * 1024 ** 3
LIVE_PATH = Path("/usr/sbin/vdr-suite-daemon")
INSTALL_LOCK = Path("/run/lock/vdr-suite-daemon-installer.lock")


class Rejected(RuntimeError):
    pass


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise Rejected(reason)


def run(*args: str, cwd: Path | None = None) -> str:
    result = subprocess.run(args, cwd=cwd, text=True, capture_output=True, check=False, timeout=45)
    require(result.returncode == 0,
            f"COMMAND_FAILED:{' '.join(args[:3])}: {result.stderr.strip()[-400:]}")
    return result.stdout.strip()


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def verify_source(root: Path, expected: str) -> None:
    require(root.is_dir() and not root.is_symlink(), "SOURCE_DIRECTORY_UNSAFE")
    require(run("git", "rev-parse", "--show-toplevel", cwd=root) == str(root.resolve()),
            "SOURCE_NOT_GIT_ROOT")
    require(run("git", "rev-parse", "HEAD", cwd=root) == expected, "SOURCE_SHA_MISMATCH")
    require(not run("git", "status", "--porcelain", "--untracked-files=no", cwd=root),
            "SOURCE_TRACKED_DIRTY")
    installer = root / "tools" / "install_daemon.py"
    require(installer.is_file() and not installer.is_symlink(), "INSTALL_TOOL_NOT_IN_CHECKOUT")
    require(digest(Path(__file__).resolve()) == digest(installer), "INSTALL_TOOL_SOURCE_MISMATCH")
    require((root / "mk" / "runtime-api-tests.mk").is_file(), "DAEMON_TARGET_MISSING")


def embedded_source(binary: Path, root: Path) -> str:
    require(binary.is_file() and not binary.is_symlink(), "LIVE_BINARY_MISSING_OR_SYMLINK")
    # Keep host memory bounded; do not trust a version merely from a file name.
    require(binary.stat().st_size <= 256 * 1024 * 1024, "BINARY_UNEXPECTED_SIZE")
    versions = set(VERSION.findall(binary.read_bytes()))
    require(len(versions) == 1, "INSTALLED_SOURCE_ID_UNKNOWN")
    prefix = next(iter(versions)).decode("ascii")
    try:
        full = run("git", "rev-parse", "--verify", prefix + "^{commit}", cwd=root)
    except Rejected as exc:
        raise Rejected("INSTALLED_SOURCE_COMMIT_NOT_LOCAL:fetch reviewed history first") from exc
    require(full.startswith(prefix), "INSTALLED_SOURCE_AMBIGUOUS")
    return full


def git_bytes(root: Path, ref: str, path: str) -> bytes:
    result = subprocess.run(["git", "show", f"{ref}:{path}"], cwd=root,
                            capture_output=True, check=False)
    return result.stdout if result.returncode == 0 else b""


def database_risk(root: Path, previous: str, target: str) -> tuple[str, list[str]]:
    """Conservative source review: never claim DB-safe for unknown provenance.

    Any changed source containing DDL/schema or deletion/migration constructs
    requires review, even if the touched lines are unrelated (false-positive
    is safer than silently skipping an actually necessary backup).
    """
    require(run("git", "merge-base", "--is-ancestor", previous, target, cwd=root) == "",
            "PREVIOUS_SOURCE_NOT_ANCESTOR")
    changed = subprocess.run(["git", "diff", "--name-only", "-z", previous, target],
                             cwd=root, capture_output=True, check=False)
    require(changed.returncode == 0, "SOURCE_DIFF_FAILED")
    flagged = []
    for pathbytes in changed.stdout.split(b"\0"):
        if not pathbytes:
            continue
        path = pathbytes.decode("utf-8")
        pl = path.lower()
        if (pl.startswith("core/sqlite/") or "/migrations/" in pl or
                "/schema/" in pl or pl.endswith(".sql")):
            flagged.append(path)
            continue
        if not (pl.endswith((".cpp", ".h", ".hpp", ".cc", ".cxx"))):
            continue
        old = git_bytes(root, previous, path)
        new = git_bytes(root, target, path)
        if SCHEMA.search(old.decode("utf-8", errors="replace")) or \
                SCHEMA.search(new.decode("utf-8", errors="replace")):
            flagged.append(path)
    return ("REVIEW_REQUIRED" if flagged else "NOT_REQUIRED", flagged)


def candidate(root: Path, sha: str) -> tuple[Path, Path, Path]:
    d = root / ".build" / "daemon-installer" / sha
    return d, d / "vdr-suite-daemon", d / "candidate.json"


def verify_binary(path: Path, sha: str) -> None:
    require(path.is_file() and not path.is_symlink(), "BUILD_BINARY_MISSING")
    require(os.access(path, os.X_OK), "BUILD_BINARY_NOT_EXECUTABLE")
    require(path.stat().st_size <= 256 * 1024 * 1024, "BUILD_BINARY_UNEXPECTED_SIZE")
    data = path.read_bytes()
    require(data.startswith(b"\x7fELF"), "BUILD_BINARY_NOT_ELF")
    require(all(feature in data for feature in FEATURES), "BUILD_DEVICE_FEATURE_MISSING")
    require(b"git-" + sha[:12].encode("ascii") in data, "BUILD_PROVENANCE_MISMATCH")


def verified_candidate(root: Path, sha: str) -> Path:
    d, binary, seal = candidate(root, sha)
    require(not any(p.is_symlink() for p in (d, d.parent, d.parent.parent)),
            "CANDIDATE_PATH_SYMLINK")
    require(seal.is_file() and not seal.is_symlink(), "NO_SEALED_BUILD_RUN_BUILD_FIRST")
    record = json.loads(seal.read_text())
    require(record == {"schema": 1, "source": sha, "binary_sha256": digest(binary)},
            "BUILD_SEAL_MISMATCH")
    verify_binary(binary, sha)
    return binary


def free_budget(path: Path, needed: int) -> None:
    location = path
    while not location.exists():
        location = location.parent
    usage = shutil.disk_usage(location)
    reserve = max(MIN_HEADROOM, (usage.total * 15 + 99) // 100)
    print(f"RESOURCE_PREFLIGHT_TOTAL={usage.total} FREE={usage.free} "
          f"NEEDED={needed} RESERVE={reserve}", flush=True)
    require(usage.free - needed >= reserve, "RESOURCE_PREFLIGHT_INSUFFICIENT")
    print("RESOURCE_PREFLIGHT=PASS", flush=True)


def build(root: Path, sha: str) -> None:
    directory, binary, seal = candidate(root, sha)
    require(not any(p.is_symlink() for p in (directory, directory.parent, directory.parent.parent)),
            "CANDIDATE_PATH_SYMLINK")
    if seal.exists():
        verified_candidate(root, sha)
        print(f"BUILD=REUSED CANDIDATE={binary}", flush=True)
        return
    if binary.exists():
        # Interrupted *after* a successful link but before sealing: certify
        # the retained artifact instead of spending another 20 minutes.
        verify_binary(binary, sha)
        payload = {"schema": 1, "source": sha, "binary_sha256": digest(binary)}
        seal.write_text(json.dumps(payload, sort_keys=True) + "\n")
        print(f"BUILD=RECOVERED_NO_COMPILE CANDIDATE={binary}", flush=True)
        return
    free_budget(directory, BUILD_BUDGET)
    directory.mkdir(parents=True, exist_ok=True, mode=0o700)
    require(not directory.is_symlink(), "UNSAFE_CANDIDATE_DIRECTORY")
    log = directory / "build.log"
    print(f"BUILD=START LOG={log}", flush=True)
    with log.open("ab") as stream:
        completed = subprocess.run(
            ["make", "-C", str(root), "daemon", f"BUILD_DIR={directory}"],
            stdout=stream, stderr=subprocess.STDOUT, check=False)
    require(completed.returncode == 0, f"BUILD_FAILED:see {log}")
    verify_binary(binary, sha)
    payload = {"schema": 1, "source": sha, "binary_sha256": digest(binary)}
    temporary = seal.with_suffix(".tmp")
    temporary.write_text(json.dumps(payload, sort_keys=True) + "\n")
    temporary.replace(seal)
    print(f"BUILD=PASS CANDIDATE={binary}", flush=True)


def service_bin_identity(service: str, live: Path) -> None:
    require(run("systemctl", "is-active", service) == "active", "SERVICE_NOT_ACTIVE")
    pid = run("systemctl", "show", "-p", "MainPID", "--value", service)
    require(pid.isdigit() and int(pid) > 0, "SERVICE_NO_PID")
    process_executable = Path("/proc") / pid / "exe"
    require(digest(process_executable) == digest(live), "RUNNING_BINARY_DIFFERS_FROM_DISK")


def atomic_replace(source: Path, target: Path, owner: os.stat_result) -> None:
    fd, temp = tempfile.mkstemp(prefix=".vdr-suite-daemon.", dir=target.parent)
    try:
        with os.fdopen(fd, "wb") as out, source.open("rb") as inp:
            shutil.copyfileobj(inp, out, length=1024 * 1024)
            out.flush()
            os.fsync(out.fileno())
        os.chown(temp, owner.st_uid, owner.st_gid)
        os.chmod(temp, stat.S_IMODE(owner.st_mode))
        os.replace(temp, target)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


def backup_database_offline(database: Path, destination: Path) -> None:
    require(database.is_file() and not database.is_symlink(), "DATABASE_PATH_UNSAFE")
    free_budget(destination, 2 * database.stat().st_size + 64 * 1024 * 1024)
    started = time.monotonic()
    def bounded_progress(_status: int, _remaining: int, _total: int) -> None:
        if time.monotonic() - started > 120:
            raise Rejected("DATABASE_BACKUP_TIMEOUT")
    try:
        source = sqlite3.connect(f"file:{database}?mode=ro", uri=True, timeout=5)
        try:
            dest = sqlite3.connect(destination, timeout=5)
            try:
                source.backup(dest, pages=256, progress=bounded_progress)
                require(dest.execute("PRAGMA quick_check").fetchone()[0] == "ok", "BACKUP_INVALID")
            finally:
                dest.close()
        finally:
            source.close()
    except BaseException:
        if destination.exists():
            destination.unlink()
        raise
    print(f"DATABASE_BACKUP=PASS PATH={destination}", flush=True)


def apply(root: Path, sha: str, args: argparse.Namespace,
          prior: str, risk: str) -> None:
    require(os.geteuid() == 0, "ROOT_REQUIRED_FOR_APPLY")
    require(args.confirm_apply, "EXPLICIT_APPLY_CONFIRMATION_REQUIRED")
    built = verified_candidate(root, sha)  # crucial: APPLY never invokes make
    require(args.live.is_file() and not args.live.is_symlink(), "UNSAFE_LIVE_BINARY")
    require(args.live.stat().st_nlink == 1, "LIVE_BINARY_HARDLINK")
    require(args.live.resolve() == LIVE_PATH,
            "LIVE_BINARY_PATH_UNEXPECTED")
    require(args.service == "vdr-suite-daemon.service",
            "SERVICE_NAME_UNEXPECTED")
    if risk == "REVIEW_REQUIRED":
        require(args.db_review in ("backup", "no-backup"), "DB_REVIEW_DECISION_REQUIRED")
        require(args.db_review_reason and len(args.db_review_reason.strip()) >= 20,
                "DB_REVIEW_RATIONALE_REQUIRED")
        if args.db_review == "backup":
            require(args.database is not None, "EXPLICIT_DATABASE_PATH_REQUIRED")
    else:
        require(args.db_review is None, "DB_BACKUP_NOT_NEEDED_REVIEW_FLAG_REFUSED")
    service_bin_identity(args.service, args.live)
    require(embedded_source(args.live, root) == prior, "LIVE_SOURCE_CHANGED")
    require(digest(args.live) != digest(built), "CANDIDATE_ALREADY_INSTALLED")
    directory, _, _ = candidate(root, sha)
    backup_binary = directory / "previous-daemon"
    if backup_binary.exists():
        require(digest(backup_binary) == digest(args.live), "ROLLBACK_BINARY_COLLISION")
    else:
        free_budget(directory, 2 * args.live.stat().st_size +
                    (2 * args.database.stat().st_size if args.db_review == "backup" else 0) +
                    64 * 1024 * 1024)
        shutil.copy2(args.live, backup_binary)
        require(digest(backup_binary) == digest(args.live), "ROLLBACK_BINARY_FAILED")
    # Cross-candidate serialization. Lock is set before service stop.
    lockfile = INSTALL_LOCK
    with lockfile.open("a+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        service_bin_identity(args.service, args.live)
        require(digest(args.live) == digest(backup_binary), "LIVE_BINARY_CHANGED_DURING_PREFLIGHT")
        before = args.live.stat()
        changed = False
        try:
            print("DAEMON_STOP=START", flush=True)
            changed = True  # even SIGINT during stop must trigger recovery
            run("systemctl", "stop", args.service)
            if args.db_review == "backup":
                db_backup = directory / "database-before.sqlite"
                require(not db_backup.exists(), "DATABASE_BACKUP_COLLISION")
                backup_database_offline(args.database, db_backup)
            else:
                print("DATABASE_BACKUP=NOT_REQUIRED", flush=True)
            atomic_replace(built, args.live, before)
            require(digest(args.live) == digest(built), "DAEMON_INSTALL_MISMATCH")
            run("systemctl", "start", args.service)
            time.sleep(3)
            service_bin_identity(args.service, args.live)
            require(digest(args.live) == digest(built), "DAEMON_POSTSTART_MISMATCH")
            print(f"DAEMON_INSTALL=PASS SOURCE_SHA={sha} RUNNING_SHA256={digest(args.live)}", flush=True)
        except BaseException:
            if changed:
                print("DAEMON_ROLLBACK=START", flush=True)
                try:
                    run("systemctl", "stop", args.service)
                    atomic_replace(backup_binary, args.live, before)
                    run("systemctl", "start", args.service)
                    service_bin_identity(args.service, args.live)
                    print("DAEMON_ROLLBACK=PASS", flush=True)
                except Exception as exc:
                    print(f"DAEMON_ROLLBACK=FAILED:{exc}", flush=True)
            raise


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "build", "apply"))
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--sha", required=True)
    parser.add_argument("--live", type=Path, default=Path("/usr/sbin/vdr-suite-daemon"))
    parser.add_argument("--service", default="vdr-suite-daemon.service")
    parser.add_argument("--confirm-apply", action="store_true")
    parser.add_argument("--db-review", choices=("backup", "no-backup"))
    parser.add_argument("--db-review-reason")
    parser.add_argument("--database", type=Path)
    args = parser.parse_args(argv)
    if args.action == "apply":
        # SIGTERM must not bypass the same guarded recovery as Ctrl+C.
        def interrupted(_number: int, _frame: object) -> None:
            raise KeyboardInterrupt("INSTALL_INTERRUPTED")
        signal.signal(signal.SIGTERM, interrupted)
    try:
        require(SHA40.fullmatch(args.sha) is not None, "SOURCE_SHA_MUST_BE_FULL_40_HEX")
        root = args.source.resolve()
        verify_source(root, args.sha)
        # Build is independent of the currently installed daemon. Failed checks
        # must never destroy a built candidate or trigger another compilation.
        if args.action == "build":
            build(root, args.sha)
            return 0
        require(args.live.is_file(), "LIVE_BINARY_MISSING")
        current = embedded_source(args.live, root)
        risk, flagged = database_risk(root, current, args.sha)
        print(f"INSTALLED_SOURCE_SHA={current}", flush=True)
        print(f"TARGET_SOURCE_SHA={args.sha}", flush=True)
        print(f"DB_BACKUP_DECISION={risk}", flush=True)
        for entry in flagged:
            print(f"DB_REVIEW_PATH={entry}", flush=True)
        directory, binary, seal = candidate(root, args.sha)
        if seal.is_file():
            verified_candidate(root, args.sha)
            print(f"BUILD=REUSABLE PATH={binary}", flush=True)
        else:
            print(f"BUILD=NOT_SEALED PATH={binary}", flush=True)
        if args.action == "apply":
            apply(root, args.sha, args, current, risk)
        else:
            print("CHECK=PASS_NO_CHANGES", flush=True)
        return 0
    except (Rejected, OSError, sqlite3.Error, subprocess.SubprocessError, ValueError, KeyboardInterrupt) as exc:
        print(f"RESULT=FAIL REASON={exc}", file=sys.stderr, flush=True)
        return 1


if __name__ == "__main__":
    sys.exit(main())
