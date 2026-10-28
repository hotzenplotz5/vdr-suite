#!/usr/bin/env python3
"""Update a mixed, existing yaVDR Media Tools installation from a sealed stage.

Unlike the exact-old-stage updater, this path records the installed bytes as
an observed, composite live snapshot. It never asserts that the snapshot was
built from one historic commit. All service control is left to the operator.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import sqlite3
import stat
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from media_tools_install_preflight import PreflightError, examined_file, safe_root
from media_tools_runtime_transaction import (
    DB_REL, database_state, live_file_ownership, live_services_quiescent,
    with_exclusive_lock,
)
from stage_media_tools_runtime import verify_media_tools_stage
from install_runtime_stage import (
    FRONTEND_ROOT, MANIFEST_NAME, DeploymentError, check, deploy, is_frontend,
    is_preserved_config, load_manifest, seal, sha256_file,
    verify_sealed_stage,
)

CONFIRM_UPDATE = "LIVE-OBSERVED-UPDATE-REVIEWED"
CONFIRM_ROLLBACK = "LIVE-OBSERVED-ROLLBACK-REVIEWED"


def require_regular_owned(path: Path, *, directory: bool = False) -> os.stat_result:
    info = path.lstat()
    correct = stat.S_ISDIR(info.st_mode) if directory else stat.S_ISREG(info.st_mode)
    if not correct or info.st_uid != 0 or (not directory and info.st_nlink != 1):
        raise PreflightError("unexpected live file/ownership: " + str(path))
    if info.st_mode & 0o022:
        raise PreflightError("group/world-writable live path: " + str(path))
    return info


def capture_observed_runtime(candidate: Path, live_root: Path, previous: Path) -> None:
    """Construct a byte-accurate rollback stage from observed live bytes.

    The input candidate defines the deployable non-frontend file set. Missing
    currently installed files are rejected instead of fabricated as 'old'.
    The complete actual live frontend is preserved, including extra files.
    """
    if previous.exists() or previous.is_symlink():
        raise PreflightError("snapshot output already exists")
    _, files = verify_sealed_stage(candidate, load_manifest(candidate))
    shutil.copytree(candidate, previous)

    for name in files:
        rel = Path(name)
        if is_preserved_config(rel) or is_frontend(rel):
            continue
        installed = examined_file(live_root, name)
        detail = installed.lstat()
        if (not stat.S_ISREG(detail.st_mode) or detail.st_nlink != 1 or
                detail.st_mode & 0o022):
            raise PreflightError("nonregular or writable installed path: " + name)
        shutil.copyfile(installed, previous / rel)
        os.chmod(previous / rel, stat.S_IMODE(detail.st_mode))

    current_frontend = examined_file(live_root, FRONTEND_ROOT.as_posix())
    require_regular_owned(current_frontend, directory=True)
    for item in current_frontend.rglob("*"):
        require_regular_owned(item, directory=item.is_dir() and not item.is_symlink())
    old_frontend = previous / FRONTEND_ROOT
    shutil.rmtree(old_frontend)
    shutil.copytree(current_frontend, old_frontend, symlinks=False)
    seal(previous)
    # The captured bytes may combine multiple source deployments and hotfixes.
    # Make the snapshot identity explicit instead of asserting one build SHA.
    snapshot_manifest = previous / MANIFEST_NAME
    payload = json.loads(snapshot_manifest.read_text(encoding="utf-8"))
    payload["producer"] = "observed-live-composite-not-a-source-commit"
    snapshot_manifest.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    check(previous, live_root)


def require_backup_directory(path: Path, *, new: bool) -> Path:
    if not path.is_absolute() or ".." in path.parts or "." in path.parts:
        raise PreflightError("backup must have an absolute canonical path")
    if path.parent.resolve() != path.parent:
        raise PreflightError("backup parent is noncanonical")
    require_regular_owned(path.parent, directory=True)
    if new:
        if path.exists() or path.is_symlink():
            raise PreflightError("backup directory already exists")
    else:
        safe_root(path)
        if stat.S_IMODE(require_regular_owned(path, directory=True).st_mode) != 0o700:
            raise PreflightError("backup must be root-private 0700")
    return path


def capacity_preflight(candidate: Path, backup_parent: Path) -> None:
    db = Path("/") / DB_REL
    db_info = db.lstat()
    if not stat.S_ISREG(db_info.st_mode) or db_info.st_nlink != 1:
        raise PreflightError("Suite database is not a regular file")
    candidate_bytes = sum(p.stat().st_size for p in candidate.rglob("*") if p.is_file())
    frontend = Path("/") / FRONTEND_ROOT
    actual_bytes = sum(p.stat().st_size for p in frontend.rglob("*") if p.is_file())
    # Two staged copies, the actual frontend, database and generous overhead.
    budget = 3 * candidate_bytes + actual_bytes + 2 * db.stat().st_size + 128 * 1024**2
    grouped: dict[int, tuple[Path, int]] = {}
    for point, need in ((backup_parent, budget), (Path("/usr"), 256 * 1024**2),
                        (Path("/var"), db.stat().st_size + 256 * 1024**2)):
        device = point.stat().st_dev
        old_point, old_need = grouped.get(device, (point, 0))
        grouped[device] = old_point, old_need + need
    for point, need in grouped.values():
        usage = shutil.disk_usage(point)
        reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
        print(f"RESOURCE_FS={point} FREE={usage.free} BUDGET={need} RESERVE={reserve}", flush=True)
        if usage.free < need + reserve:
            raise PreflightError("RESOURCE_PREFLIGHT=STOP")
    print("RESOURCE_PREFLIGHT=PASS", flush=True)


def write_review(path: Path, evidence: dict) -> None:
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
    with os.fdopen(fd, "w", encoding="utf-8") as stream:
        stream.write(json.dumps(evidence, indent=2, sort_keys=True) + "\n")
        stream.flush()
        os.fsync(stream.fileno())


def update(candidate: Path, digest: str, backup_dir: Path) -> str:
    if os.geteuid() != 0:
        raise PreflightError("live update requires root")
    candidate = safe_root(candidate)
    verify_media_tools_stage(candidate, digest)
    require_backup_directory(backup_dir, new=True)
    live_services_quiescent()
    # Verify ownership and dpkg boundaries, not a fictitious previous SHA.
    live_file_ownership(candidate)
    with with_exclusive_lock(Path("/")):
        live_services_quiescent()
        live_file_ownership(candidate)
        capacity_preflight(candidate, backup_dir.parent)
        backup_dir.mkdir(mode=0o700)
        private_candidate = backup_dir / "candidate-stage"
        private_previous = backup_dir / "observed-previous-stage"
        shutil.copytree(candidate, private_candidate)
        verify_media_tools_stage(private_candidate, digest)
        capture_observed_runtime(private_candidate, Path("/"), private_previous)

        db = Path("/") / DB_REL
        saved_db = backup_dir / "vdr-suite.db"
        with sqlite3.connect(db.as_uri() + "?mode=ro", uri=True) as source:
            with sqlite3.connect(saved_db) as destination:
                source.backup(destination)
        if saved_db.stat().st_size <= 0:
            raise PreflightError("SQLite backup is empty")
        os.chmod(saved_db, 0o600)
        evidence = {
            "schema": 1,
            "snapshot_kind": "observed-live-composite-not-a-source-commit",
            "candidate_handler_sha256": digest,
            "previous_manifest_sha256": sha256_file(
                private_previous / ".vdr-suite-install-runtime-manifest.json"),
            "candidate_manifest_sha256": sha256_file(
                private_candidate / ".vdr-suite-install-runtime-manifest.json"),
            "database_state": database_state(),
            "services": "confirmed_inactive",
        }
        write_review(backup_dir / "review.json", evidence)
        live_services_quiescent()
        check(private_previous, Path("/"))
        try:
            deploy(private_candidate, Path("/"))
            check(private_candidate, Path("/"))
        except Exception as failure:
            try:
                deploy(private_previous, Path("/"))
                check(private_previous, Path("/"))
            except Exception as restore_failure:
                raise PreflightError(
                    "UPDATE_FAILED; ROLLBACK_FAILED; DO_NOT_START_SERVICES: " +
                    str(restore_failure)) from failure
            raise PreflightError(
                "UPDATE_FAILED; ORIGINAL_LIVE_BYTES_RESTORED; SERVICES_STOPPED: " +
                str(failure)) from failure
    return ("MEDIA_TOOLS_OBSERVED_LIVE_UPDATE=PASS BACKUP=" + str(backup_dir) +
            " SERVICES=STOPPED GRANTS=UNCHANGED")


def rollback(backup_dir: Path, digest: str) -> str:
    if os.geteuid() != 0:
        raise PreflightError("live rollback requires root")
    require_backup_directory(backup_dir, new=False)
    review_path = backup_dir / "review.json"
    if stat.S_IMODE(require_regular_owned(review_path).st_mode) != 0o600:
        raise PreflightError("rollback evidence is not root-private")
    evidence = json.loads(review_path.read_text(encoding="utf-8"))
    if (evidence.get("schema") != 1 or evidence.get("services") != "confirmed_inactive" or
            evidence.get("snapshot_kind") != "observed-live-composite-not-a-source-commit" or
            evidence.get("candidate_handler_sha256") != digest):
        raise PreflightError("unrecognized rollback evidence")
    previous = backup_dir / "observed-previous-stage"
    candidate = backup_dir / "candidate-stage"
    verify_sealed_stage(previous, load_manifest(previous))
    verify_media_tools_stage(candidate, digest)
    for stage, key in ((previous, "previous_manifest_sha256"),
                       (candidate, "candidate_manifest_sha256")):
        if sha256_file(stage / ".vdr-suite-install-runtime-manifest.json") != evidence.get(key):
            raise PreflightError("backup manifest differs from snapshot evidence")
    live_services_quiescent()
    with with_exclusive_lock(Path("/")):
        live_services_quiescent()
        if database_state() != evidence.get("database_state"):
            raise PreflightError("SQLite state has changed; rollback requires review")
        check(candidate, Path("/"))
        live_file_ownership(candidate)
        try:
            deploy(previous, Path("/"))
            check(previous, Path("/"))
        except Exception as failure:
            try:
                deploy(candidate, Path("/"))
                check(candidate, Path("/"))
            except Exception as recovery_failure:
                raise PreflightError(
                    "ROLLBACK_FAILED; RECOVERY_FAILED; DO_NOT_START_SERVICES: " +
                    str(recovery_failure)) from failure
            raise PreflightError("ROLLBACK_FAILED; CANDIDATE_RESTORED: " +
                                 str(failure)) from failure
    return ("MEDIA_TOOLS_OBSERVED_LIVE_ROLLBACK=PASS BACKUP=" +
            str(backup_dir) + " SERVICES=STOPPED")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("live-update", "live-rollback"))
    parser.add_argument("--candidate-stage", type=Path)
    parser.add_argument("--approved-handler-sha256", required=True)
    parser.add_argument("--backup-dir", type=Path, required=True)
    parser.add_argument("--confirm", required=True)
    args = parser.parse_args()
    if args.action == "live-update" and (
        args.confirm != CONFIRM_UPDATE or args.candidate_stage is None
    ):
        parser.error("live-update needs --candidate-stage and " + CONFIRM_UPDATE)
    if args.action == "live-rollback" and (
        args.confirm != CONFIRM_ROLLBACK or args.candidate_stage is not None
    ):
        parser.error("live-rollback needs " + CONFIRM_ROLLBACK)
    try:
        if args.action == "live-update":
            result = update(args.candidate_stage, args.approved_handler_sha256,
                            args.backup_dir)
        else:
            result = rollback(args.backup_dir, args.approved_handler_sha256)
        print(result)
        return 0
    except (OSError, ValueError, RuntimeError, DeploymentError) as exc:
        print("MEDIA_TOOLS_OBSERVED_TRANSACTION=STOP " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
