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
import os
from pathlib import Path
import stat
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("update", "rollback"))
    parser.add_argument("--previous-stage", type=Path, required=True)
    parser.add_argument("--new-stage", type=Path, required=True)
    parser.add_argument("--live-root", type=Path, required=True)
    parser.add_argument("--previous-approved-sha256", required=True)
    parser.add_argument("--new-approved-sha256", required=True)
    parser.add_argument("--confirm", required=True,
                        help="must equal ISOLATED-ONLY")
    args = parser.parse_args()
    if args.confirm != "ISOLATED-ONLY":
        parser.error("--confirm must equal ISOLATED-ONLY; live installation is forbidden")
    try:
        result = transaction(args.action, args.previous_stage, args.new_stage,
                             args.live_root, args.previous_approved_sha256,
                             args.new_approved_sha256)
        print(result)
        print("REAL_VDR=UNCHANGED; AGENT=NOT_STARTED; APPROVAL=NOT_ISSUED")
        return 0
    except (OSError, ValueError, DeploymentError, RuntimeError) as exc:
        print("MEDIA_TOOLS_TRANSACTION=STOP " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
