#!/usr/bin/env python3
"""MT.I2 isolated update/rollback rehearsal using the existing Suite runtime deploy.

The maintenance adapter intentionally refuses the real '/' root. A real
yaVDR update additionally needs stopped-service/Agent-drain/DB-backup and
root-owned immutable staged-source gates, which are NOT accepted here.
No new file-deployment logic or add-on authorization is introduced.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import stat
import subprocess
import sys

from stage_media_tools_runtime import verify_media_tools_stage
from media_tools_install_preflight import (
    PreflightError, examined_file, safe_root,
)
from install_runtime_stage import (
    DeploymentError, check, deploy, load_manifest, verify_sealed_stage,
)

MARKER = ".vdr-suite-mt-i2-test-root"
RUN_LOCK = "etc/vdr-suite/addon-approvals/rectools.import-running.lock"


def synthetic_only(root: Path) -> Path:
    root = safe_root(root)
    if root == Path("/") or os.geteuid() == 0:
        raise PreflightError("live root and privileged update are NOT supported")
    info = root.lstat()
    if info.st_uid != os.geteuid() or stat.S_IMODE(info.st_mode) != 0o700:
        raise PreflightError("synthetic root must be private and owned by tester")
    marker = examined_file(root, MARKER)
    try:
        details = marker.lstat()
        if (not stat.S_ISREG(details.st_mode) or
                details.st_uid != os.geteuid() or
                stat.S_IMODE(details.st_mode) != 0o600 or
                marker.read_text(encoding="ascii") != "MT_I2_SYNTHETIC_ROOT\n"):
            raise PreflightError("synthetic-root marker invalid")
    except FileNotFoundError as exc:
        raise PreflightError("synthetic-root marker missing") from exc
    return root


def check_stage_pair(old: Path, new: Path, old_digest: str,
                     new_digest: str) -> None:
    old = safe_root(old)
    new = safe_root(new)
    if old == new:
        raise PreflightError("previous and candidate stage must be distinct")
    verify_media_tools_stage(old, old_digest)
    verify_media_tools_stage(new, new_digest)
    previous_dirs, previous_files = verify_sealed_stage(old, load_manifest(old))
    candidate_dirs, candidate_files = verify_sealed_stage(new, load_manifest(new))
    if previous_dirs != candidate_dirs or (
            set(previous_files) != set(candidate_files)):
        raise PreflightError("stage tree shape differs; needs explicit migration")
    # Existing host directory and installed-file modes are immutable here.
    if any(previous_files[path]["mode"] != candidate_files[path]["mode"]
           for path in previous_files):
        raise PreflightError("staged file modes differ; needs explicit migration")


def with_exclusive_lock(root: Path):
    import contextlib

    @contextlib.contextmanager
    def held():
        lock = examined_file(root, RUN_LOCK)
        try:
            detail = lock.lstat()
        except FileNotFoundError as exc:
            raise PreflightError("maintenance lock missing") from exc
        if (not stat.S_ISREG(detail.st_mode) or detail.st_nlink != 1 or
                detail.st_uid != os.geteuid() or
                stat.S_IMODE(detail.st_mode) != 0o644 or detail.st_size != 0):
            raise PreflightError("unsafe maintenance lock")
        fd = os.open(lock, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
        try:
            if os.fstat(fd).st_ino != detail.st_ino:
                raise PreflightError("maintenance lock changed")
            try:
                fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
            except BlockingIOError as exc:
                raise PreflightError("maintenance lock held by another task") from exc
            yield
        finally:
            os.close(fd)
    return held()


def transaction(action: str, old: Path, new: Path, root: Path,
                old_digest: str, new_digest: str) -> str:
    """Apply or revert only in a deliberately marked, disposable test root."""
    root = synthetic_only(root)
    check_stage_pair(old, new, old_digest, new_digest)
    with with_exclusive_lock(root):
        if action == "update":
            check(old, root)
            try:
                # One production deploy implementation; no replacement copy.
                deploy(new, root)
                check(new, root)
                return "MEDIA_TOOLS_ISOLATED_UPDATE=PASS"
            except Exception as exc:
                # The reviewed previous sealed stage is the exact byte-level
                # backup; directory/file shape equality ensures reversibility.
                try:
                    deploy(old, root)
                    check(old, root)
                except Exception as rollback_exc:
                    raise PreflightError(
                        "UPDATE_FAILED; ROLLBACK_FAILED; DO_NOT_RESTART: " +
                        str(rollback_exc)) from exc
                raise PreflightError(
                    "UPDATE_FAILED; AUTOMATIC_ROLLBACK=PASS: " + str(exc)) from exc
        if action == "rollback":
            check(new, root)
            deploy(old, root)
            check(old, root)
            return "MEDIA_TOOLS_ISOLATED_ROLLBACK=PASS"
    raise PreflightError("unknown transaction action")



# The live adapter below remains separate from the synthetic rehearsal, but
# reuses exactly the same checks, lock, and Suite-owned deploy/check functions.
# It never changes file ownership/modes in place, Agent grants, or services.
DB_REL = Path("var/lib/vdr-suite/vdr-suite.db")
SERVICES = ("vdr-suite-daemon", "vdr-suite-backend-agent")


def live_services_quiescent() -> None:
    for service in SERVICES:
        def field(name: str) -> str:
            result = subprocess.run(
                ["systemctl", "show", "-p", name, "--value", service],
                capture_output=True, text=True, check=True, timeout=10)
            return result.stdout.strip()
        if (field("LoadState") != "loaded" or
                field("ActiveState") != "inactive" or
                field("Result") != "success"):
            raise PreflightError(
                "service must be cleanly stopped before maintenance: " + service)


def live_file_ownership(stage: Path) -> None:
    _, files = verify_sealed_stage(stage, load_manifest(stage))
    for rel, entry in files.items():
        if rel.startswith("etc/"):
            continue  # preserved configuration is not installed or replaced
        target = examined_file(Path("/"), rel)
        details = target.lstat()
        if (not stat.S_ISREG(details.st_mode) or details.st_nlink != 1 or
                details.st_uid != 0 or
                stat.S_IMODE(details.st_mode) != entry["mode"]):
            raise PreflightError("unsafe existing ownership/mode: " + rel)
        # Never overlay a dpkg-owned file with a manual source deployment.
        query = subprocess.run(["dpkg-query", "-S", "/" + rel],
                               capture_output=True, text=True, timeout=5)
        if query.returncode == 0:
            raise PreflightError("Debian-owned file needs package update: " + rel)
        if query.returncode != 1:
            raise PreflightError("package ownership query unavailable: " + rel)


def live_backup_directory(path: Path, old: Path, new: Path) -> Path:
    if not path.is_absolute() or path.is_symlink() or path.exists():
        raise PreflightError("backup directory must be a new absolute path")
    parent = safe_root(path.parent)
    info = parent.lstat()
    if info.st_uid != 0 or info.st_mode & 0o022:
        raise PreflightError("backup parent must be root-owned and not writable by others")
    database = Path("/") / DB_REL
    db_info = database.lstat()
    if not stat.S_ISREG(db_info.st_mode) or db_info.st_size <= 0:
        raise PreflightError("missing or unsafe Suite database")
    need = (sum(p.stat().st_size for stage in (old, new)
                for p in stage.rglob("*") if p.is_file()) +
            2 * db_info.st_size + 64 * 1024 * 1024)
    for point in (parent, Path("/usr"), Path("/var")):
        disk = shutil.disk_usage(point)
        reserve = max(4 * 1024**3, (disk.total * 15 + 99) // 100)
        if disk.free < need + reserve:
            raise PreflightError("insufficient storage reserve for staged backup")
    path.mkdir(mode=0o700)
    return path


def stable_sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def live_update(old: Path, new: Path, old_digest: str, new_digest: str,
                backup_dir: Path) -> str:
    if os.geteuid() != 0:
        raise PreflightError("live update requires separately approved root invocation")
    check_stage_pair(old, new, old_digest, new_digest)
    live_services_quiescent()
    # A full exact previous deployment is required: never guess backup bytes.
    check(old, Path("/"))
    live_file_ownership(old)
    with with_exclusive_lock(Path("/")):
        live_services_quiescent()
        check(old, Path("/"))
        root_backup = live_backup_directory(backup_dir, old, new)
        # Failures before deployment retain backup for investigation; never
        # chown/chmod anything that already existed on the production host.
        try:
            previous = root_backup / "previous-stage"
            candidate = root_backup / "candidate-stage"
            shutil.copytree(old, previous)
            shutil.copytree(new, candidate)
            check_stage_pair(previous, candidate, old_digest, new_digest)
            database = Path("/") / DB_REL
            backup_db = root_backup / "vdr-suite.db"
            with sqlite3.connect(database.as_uri() + "?mode=ro", uri=True) as source:
                with sqlite3.connect(backup_db) as destination:
                    source.backup(destination)
            if backup_db.stat().st_size == 0:
                raise PreflightError("empty SQLite backup")
            evidence = {
                "schema": 1, "previous_approved_sha256": old_digest,
                "candidate_approved_sha256": new_digest,
                "database_sha256": stable_sha(database),
                "services": "confirmed_inactive",
            }
            (root_backup / "review.json").write_text(
                json.dumps(evidence, sort_keys=True) + "\n", encoding="utf-8")
            live_services_quiescent()
            check(old, Path("/"))
            try:
                deploy(candidate, Path("/"))
                check(candidate, Path("/"))
            except Exception as original:
                try:
                    deploy(previous, Path("/"))
                    check(previous, Path("/"))
                except Exception as restore:
                    raise PreflightError(
                        "LIVE_UPDATE_FAILED; ROLLBACK_FAILED; SERVICES_MUST_STAY_STOPPED: " +
                        str(restore)) from original
                raise PreflightError(
                    "LIVE_UPDATE_FAILED; ROLLBACK_VERIFIED; SERVICES_STAY_STOPPED: " +
                    str(original)) from original
        except Exception:
            # Retain backup and evidence; never start services automatically.
            raise
    return ("MEDIA_TOOLS_LIVE_FILE_UPDATE=PASS BACKUP=" + str(root_backup) +
            " SERVICES=STOPPED GRANTS=UNCHANGED")



def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("update", "rollback", "live-update"))
    parser.add_argument("--previous-stage", type=Path, required=True)
    parser.add_argument("--new-stage", type=Path, required=True)
    parser.add_argument("--live-root", type=Path, required=True)
    parser.add_argument("--previous-approved-sha256", required=True)
    parser.add_argument("--new-approved-sha256", required=True)
    parser.add_argument("--backup-dir", type=Path)
    parser.add_argument("--confirm", required=True,
                        help="ISOLATED-ONLY or LIVE-UPDATE-REVIEWED")
    args = parser.parse_args()
    if args.action == "live-update":
        if (args.confirm != "LIVE-UPDATE-REVIEWED" or
                args.live_root != Path("/") or args.backup_dir is None):
            parser.error("live-update requires root '/', a new --backup-dir and LIVE-UPDATE-REVIEWED")
    elif args.confirm != "ISOLATED-ONLY" or args.backup_dir is not None:
        parser.error("--confirm must equal ISOLATED-ONLY; live installation is forbidden")
    try:
        if args.action == "live-update":
            result = live_update(args.previous_stage, args.new_stage,
                                 args.previous_approved_sha256,
                                 args.new_approved_sha256, args.backup_dir)
        else:
            result = transaction(args.action, args.previous_stage, args.new_stage,
                                 args.live_root, args.previous_approved_sha256,
                                 args.new_approved_sha256)
        print(result)
        if args.action == "live-update":
            print("SERVICES=STILL_STOPPED; APPROVAL_AND_ACTIVATION=UNCHANGED")
        else:
            print("REAL_VDR=UNCHANGED; AGENT=NOT_STARTED; APPROVAL=NOT_ISSUED")
        return 0
    except (OSError, ValueError, DeploymentError, RuntimeError) as exc:
        print("MEDIA_TOOLS_TRANSACTION=STOP " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
