#!/usr/bin/env python3
"""Install only a reviewed, sealed Media Tools payload; no init-system dependency.

Never deploy a Suite daemon, Agent, frontend, config, database, media file or
approval. Source staging/compilation is a separate unprivileged Make target.
For live changes requiring worker replacement the operator must attest
quiescence; no systemd, init scripts, package manager or service restart here.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile

from stage_media_tools_runtime import (
    INVENTORY_REL, WORKER_REL, payload_roles, verify_media_tools_stage,
)

class InstallError(RuntimeError):
    pass

def ensure(condition: bool, reason: str) -> None:
    if not condition:
        raise InstallError(reason)

def hash_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for part in iter(lambda: f.read(1024 * 1024), b""):
            h.update(part)
    return h.hexdigest()

def controlled_parent(root: Path, rel: Path, *, create: bool) -> Path:
    current = root
    for name in rel.parts[:-1]:
        current = current / name
        try:
            info = current.lstat()
        except FileNotFoundError:
            ensure(create, "missing destination parent " + str(current))
            current.mkdir(mode=0o755)
            info = current.lstat()
        ensure(stat.S_ISDIR(info.st_mode), "unsafe destination parent " + str(current))
        # Existing permissions belong to the distribution. Do not modify them.
        if root == Path("/"):
            ensure(info.st_uid == 0 and not info.st_mode & 0o022,
                   "unsafe root-owned destination parent " + str(current))
    return current / rel.name

def file_info(path: Path, owner: int):
    try:
        info = path.lstat()
    except FileNotFoundError:
        return None
    ensure(stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and
           info.st_uid == owner and not info.st_mode & 0o022,
           "unsafe existing payload " + str(path))
    return info

def package_owner(path: Path) -> bool:
    # Never overwrite another package manager's manually installed path.
    probes = (
        ("dpkg-query", "-S", str(path)),
        ("rpm", "-qf", str(path)),
        ("pacman", "-Qo", str(path)),
        ("apk", "info", "--who-owns", str(path)),
    )
    for args in probes:
        if shutil.which(args[0]) is None:
            continue
        result = subprocess.run(args, text=True, capture_output=True,
                                timeout=7, check=False)
        if result.returncode == 0 and result.stdout.strip():
            return True
    return False

def copy_atomic(source: Path, target: Path, mode: int) -> None:
    parent = target.parent
    fd, raw = tempfile.mkstemp(prefix=".media-tools-replace-", dir=parent)
    temporary = Path(raw)
    try:
        with os.fdopen(fd, "wb") as output, source.open("rb") as inp:
            shutil.copyfileobj(inp, output)
            output.flush()
            os.fsync(output.fileno())
        os.chmod(temporary, mode)
        os.replace(temporary, target)
    finally:
        if temporary.exists():
            temporary.unlink()

APPROVAL_REL = Path("etc/vdr-suite/addon-approvals/rectools.json")

def approval_gate(root: Path, approved: str, owner: int, *,
                  previous_sha256: str = "", rotate: bool = False):
    """Review a root approval for explicit pin rotation; no writes."""
    path = root / APPROVAL_REL
    info = file_info(path, owner)
    if info is None:
        ensure(not rotate and not previous_sha256, "cannot rotate absent root approval")
        return None
    ensure(0 < info.st_size <= 4096, "invalid root approval size")
    try:
        original = path.read_bytes()
        approval = json.loads(original)
    except (OSError, UnicodeError, ValueError) as exc:
        raise InstallError("invalid existing root approval") from exc
    ensure(isinstance(approval, dict) and
           approval.get("id") == "rectools" and
           approval.get("package") == "vdr-suite-addon-media-tools" and
           approval.get("state") == "approved",
           "root approval is not a supported approved Media Tools record")
    if approval.get("sha256") == approved:
        ensure(not rotate and not previous_sha256, "rotation not required")
        return None
    ensure(rotate and len(previous_sha256) == 64 and
           all(ch in "0123456789abcdef" for ch in previous_sha256),
           "root approval differs; explicit reviewed update required")
    ensure(approval.get("sha256") == previous_sha256,
           "old approved handler digest does not match")
    ensure(set(approval) == {"id", "origin", "package", "schemaVersion",
                            "sha256", "state", "version"} and
           approval.get("schemaVersion") == 1 and
           approval.get("origin") == "manual",
           "only canonical manual root approvals can be rotated")
    ensure(original == (json.dumps(approval, sort_keys=True,
             separators=(",", ":")) + "\n").encode("utf-8"),
           "root approval is noncanonical")
    old_handler = root / WORKER_REL / "vdr-suite-media-import"
    ensure(file_info(old_handler, owner) is not None and
           hash_file(old_handler) == previous_sha256,
           "installed old handler bytes do not match old approval")
    ensure(not any(path.parent.glob("rectools.*.json")),
           "backend-scoped approvals require separate migration")
    successor = dict(approval)
    successor["sha256"] = approved
    return (json.dumps(successor, sort_keys=True,
                       separators=(",", ":")) + "\n").encode("utf-8")


def install(stage: Path, root: Path, approved: str, *, quiesced: bool = False,
            require_quiescence: bool | None = None,
            previous_sha256: str = "", rotate_approval: bool = False) -> str:
    ensure(stage.is_absolute() and stage.is_dir() and not stage.is_symlink(),
           "absolute sealed stage required")
    ensure(root.is_absolute() and root.is_dir() and not root.is_symlink(),
           "absolute destination root required")
    ensure(root == root.resolve() and stage == stage.resolve(),
           "symlinked or noncanonical install roots")
    if root == Path("/"):
        ensure(os.geteuid() == 0, "live installation requires root")
    owner = 0 if root == Path("/") else os.geteuid()
    record = verify_media_tools_stage(stage, approved)
    new_approval = approval_gate(root, approved, owner,
                                 previous_sha256=previous_sha256,
                                 rotate=rotate_approval)

    entries = record["files"]
    paths = sorted((*payload_roles().keys(), INVENTORY_REL.as_posix()))
    desired = {}
    changed = []
    worker_changes = []
    for raw in paths:
        rel = Path(raw)
        target = controlled_parent(root, rel, create=False) if (root / rel).parent.exists() else root / rel
        source = stage / rel
        entry = entries.get(raw)
        if raw == INVENTORY_REL.as_posix():
            # The inventory is separately sealed by the entire Suite stage.
            inventory = stage / INVENTORY_REL
            entry = {"sha256": hash_file(inventory),
                     "size": inventory.stat().st_size, "mode": 0o644}
        ensure(isinstance(entry, dict), "incomplete add-on payload")
        info = file_info(target, owner) if target.exists() or target.is_symlink() else None
        if info is not None and root == Path("/") and package_owner(target):
            raise InstallError("dpkg/rpm/pacman/apk owns installed file: " + raw)
        desired[raw] = entry
        if info is None or (info.st_size != entry["size"] or
                           stat.S_IMODE(info.st_mode) != entry["mode"] or
                           hash_file(target) != entry["sha256"]):
            changed.append(raw)
            if rel.parts[:len(WORKER_REL.parts)] == WORKER_REL.parts and rel.name.startswith("vdr-suite-media-"):
                worker_changes.append(raw)

    if new_approval is not None:
        grant = APPROVAL_REL.as_posix()
        desired[grant] = {
            "sha256": hashlib.sha256(new_approval).hexdigest(),
            "size": len(new_approval),
            "mode": stat.S_IMODE((root / APPROVAL_REL).stat().st_mode),
        }
        changed.append(grant)
    if not changed:
        return "MEDIA_TOOLS_INSTALL=ALREADY_CURRENT FILES=" + str(len(paths))
    if require_quiescence is None:
        require_quiescence = root == Path("/")
    if require_quiescence and (worker_changes or new_approval is not None):
        ensure(quiesced, "worker replacement requires operator-confirmed quiescence (MEDIA_TOOLS_QUIESCED=YES)")

    lock_path = root / "etc/vdr-suite/addon-approvals/rectools.import-running.lock"
    lock_fd = None
    if lock_path.exists() or lock_path.is_symlink():
        detail = file_info(lock_path, owner)
        ensure(detail is not None and detail.st_size == 0,
               "unsafe existing Media Tools lock")
        lock_fd = os.open(lock_path, os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC)
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as exc:
            os.close(lock_fd)
            raise InstallError("Media Tools operation active; installation denied") from exc

    try:
        if new_approval is not None:
            ensure(lock_fd is not None,
                   "approval rotation requires existing exclusive Media Tools lock")
            ensure(new_approval == approval_gate(
                root, approved, owner, previous_sha256=previous_sha256,
                rotate=True), "root approval changed after obtaining lock")
        backup_parent = root / "root"
        ensure(backup_parent.is_dir() and not backup_parent.is_symlink(),
               "private backup parent missing")
        if root == Path("/"):
            info = backup_parent.stat()
            ensure(info.st_uid == 0 and not info.st_mode & 0o022,
                   "unsafe backup parent")
        backup = Path(tempfile.mkdtemp(prefix="vdr-suite-media-payload-", dir=backup_parent))
        backup.chmod(0o700)
        retained = backup / "previous"
        reviewed = backup / "reviewed"
        retained.mkdir(mode=0o700)
        reviewed.mkdir(mode=0o700)
        existed = []
        try:
            # Freeze the source bytes before making any live modifications.
            for raw in changed:
                rel = Path(raw)
                source = stage / rel
                entry = desired[raw]
                frozen = reviewed / rel
                frozen.parent.mkdir(parents=True, exist_ok=True)
                if rel == APPROVAL_REL:
                    frozen.write_bytes(new_approval)
                else:
                    shutil.copyfile(source, frozen)
                frozen.chmod(int(entry["mode"]))
                ensure(hash_file(frozen) == entry["sha256"], "staging source drift: " + raw)
                target = root / rel
                if target.exists():
                    old = retained / rel
                    old.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copyfile(target, old)
                    old.chmod(stat.S_IMODE(target.stat().st_mode))
                    existed.append(raw)
            evidence = {"schema": 1, "changed": changed, "previous": existed,
                        "approved_handler_sha256": approved,
                        "source_commit": record["prepared_source_sha"],
                        "service_state": "UNCHANGED",
                        "approval_state": ("ROTATED_WITH_BACKUP" if new_approval is not None else "UNCHANGED"),
                        "previous_handler_sha256": previous_sha256 or None}
            (backup / "review.json").write_text(
                json.dumps(evidence, sort_keys=True) + "\n", encoding="utf-8")
            (backup / "review.json").chmod(0o600)
            applied = []
            try:
                for raw in changed:
                    rel = Path(raw)
                    target = controlled_parent(root, rel, create=True)
                    # Refuse a new symlink or replacement after preflight.
                    file_info(target, owner)
                    copy_atomic(reviewed / rel, target, int(desired[raw]["mode"]))
                    applied.append(raw)
                    ensure(hash_file(target) == desired[raw]["sha256"],
                           "post-install mismatch: " + raw)
            except Exception as exc:
                rollback_error = None
                for raw in reversed(applied):
                    rel = Path(raw)
                    try:
                        if raw in existed:
                            previous = retained / rel
                            copy_atomic(previous, root / rel,
                                        stat.S_IMODE(previous.stat().st_mode))
                        else:
                            (root / rel).unlink()
                    except Exception as rollback_exc:
                        rollback_error = rollback_exc
                if rollback_error is not None:
                    raise InstallError("INSTALL_FAILED; ROLLBACK_FAILED; backup=" + str(backup)) from exc
                raise InstallError("INSTALL_FAILED; RESTORED_PREVIOUS_FILES; backup=" + str(backup)) from exc
        except Exception:
            # Backups are retained for manual review after any failure.
            raise
        return ("MEDIA_TOOLS_INSTALL=PASS FILES_UPDATED=" + str(len(changed)) +
                " BACKUP=" + str(backup) +
                " SERVICES=UNCHANGED APPROVAL=" +
                ("ROTATED" if new_approval is not None else "UNCHANGED"))
    finally:
        if lock_fd is not None:
            os.close(lock_fd)

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sealed-stage", type=Path, required=True)
    parser.add_argument("--approved-handler-sha256", required=True)
    parser.add_argument("--live-root", type=Path, default=Path("/"))
    parser.add_argument("--previous-approved-handler-sha256", default="")
    args = parser.parse_args()
    try:
        print(install(args.sealed_stage, args.live_root,
                      args.approved_handler_sha256,
                      quiesced=os.environ.get("MEDIA_TOOLS_QUIESCED") == "YES",
                      previous_sha256=args.previous_approved_handler_sha256,
                      rotate_approval=(os.environ.get("MEDIA_TOOLS_APPROVAL_ROTATION")
                                       == "REVIEWED_UPDATE")))
        return 0
    except (InstallError, ValueError, OSError, RuntimeError,
            subprocess.TimeoutExpired) as exc:
        print("MEDIA_TOOLS_INSTALL=STOP " + str(exc), file=sys.stderr)
        return 1

if __name__ == "__main__":
    raise SystemExit(main())
