#!/usr/bin/env python3
"""Keep two EXISTING manual Media Tools approvals consistent after a handler-only update.

This does not install a handler, create an approval, activate the add-on,
change actor rights, or restart services. The expected digest is supplied
independently and the actual installed handler is checked before any write.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import stat
import sys
import tempfile

from admin_approval_issuer import IssueError, rotate
from manual_handler_evidence import examine as examine_handler

_SHA = re.compile(r"[0-9a-f]{64}\Z")
_BACKEND = re.compile(r"[A-Za-z0-9_.:-]{1,128}\Z")
_KEYS = {"schemaVersion", "id", "package", "version", "sha256", "origin", "state"}


class ReconcileError(Exception):
    pass


def read_existing(path: Path) -> dict:
    """Read only an existing, regular root-controlled approval; never create one."""
    try:
        meta = path.lstat()
        if (not stat.S_ISREG(meta.st_mode) or meta.st_uid != 0 or
                meta.st_mode & 0o022 or meta.st_nlink != 1 or
                not 0 < meta.st_size <= 4096):
            raise ReconcileError("unsafe-existing-approval: " + str(path))

        def unique(pairs):
            result = {}
            for key, value in pairs:
                if key in result:
                    raise ReconcileError("duplicate-approval-key: " + str(path))
                result[key] = value
            return result

        record = json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=unique)
    except (OSError, ValueError, UnicodeError) as exc:
        raise ReconcileError("unreadable-existing-approval: " + str(path)) from exc

    if (not isinstance(record, dict) or set(record) != _KEYS or
            type(record["schemaVersion"]) is not int or record["schemaVersion"] != 1 or
            record["id"] != "rectools" or
            record["package"] != "vdr-suite-addon-media-tools" or
            record["version"] != "0.1.0" or record["origin"] != "manual" or
            record["state"] != "approved" or
            not isinstance(record["sha256"], str) or not _SHA.fullmatch(record["sha256"])):
        raise ReconcileError("non-approved-or-invalid-record: " + str(path))
    return record


def approval_paths(root: Path, backend_id: str) -> tuple[Path, Path]:
    if (not _BACKEND.fullmatch(backend_id) or backend_id in (".", "..") or
            not root.is_absolute() or "." in root.parts or ".." in root.parts):
        raise ReconcileError("invalid-root-or-backend")
    base = root / "etc/vdr-suite/addon-approvals"
    return base / "rectools.json", base / ("rectools." + backend_id + ".json")


def plan(root: Path, backend_id: str, reviewed_digest: str) -> list[tuple[Path, str]]:
    if not _SHA.fullmatch(reviewed_digest):
        raise ReconcileError("invalid-reviewed-handler-digest")
    global_file, backend_file = approval_paths(root, backend_id)
    # Both approval records must already exist and be approved. No new grant.
    records = [(global_file, read_existing(global_file)),
               (backend_file, read_existing(backend_file))]
    return [(path, record["sha256"]) for path, record in records
            if record["sha256"] != reviewed_digest]


def reconcile(root: Path, backend_id: str, reviewed_digest: str,
              backup_parent: Path = Path("/root")) -> str:
    changes = plan(root, backend_id, reviewed_digest)
    evidence = examine_handler(root, "rectools", reviewed_digest)
    if not evidence["consistent"]:
        raise ReconcileError("installed-handler-" + evidence["reason"])
    if not changes:
        return "MT_APPROVALS=ALREADY_CONSISTENT"

    # Preserve both original records before rotation, even if only one differs.
    backup = Path(tempfile.mkdtemp(prefix="mt-approval-sync-", dir=backup_parent))
    global_file, backend_file = approval_paths(root, backend_id)
    for path in (global_file, backend_file):
        shutil.copy2(path, backup / path.name, follow_symlinks=False)

    # Rotate backend first: a failed global rotation can be compensated through
    # the existing backend-only CAS issuer without needing the old binary.
    backend_old = next((old for path, old in changes if path == backend_file), None)
    backend_rotated = False
    try:
        if backend_old is not None:
            rotate(root, "rectools", "0.1.0", backend_old, reviewed_digest,
                   "manual", allow_live_root=True, core_expected_only=True,
                   backend_id=backend_id)
            backend_rotated = True
        for path, old in changes:
            if path == global_file:
                rotate(root, "rectools", "0.1.0", old, reviewed_digest, "manual",
                       allow_live_root=True)
        if plan(root, backend_id, reviewed_digest):
            raise ReconcileError("approval-postcheck-mismatch")
    except (IssueError, ReconcileError, OSError) as exc:
        if backend_rotated:
            try:
                # Only undo the backend rotation if the global approval failed
                # and is not already at the new digest.
                if read_existing(global_file)["sha256"] != reviewed_digest:
                    rotate(root, "rectools", "0.1.0", reviewed_digest, backend_old,
                           "manual", allow_live_root=True, core_expected_only=True,
                           backend_id=backend_id)
            except (IssueError, ReconcileError, OSError) as rollback_exc:
                raise ReconcileError(
                    f"rotation-failed; backend-rollback-failed={rollback_exc}; backup={backup}"
                ) from exc
        raise ReconcileError(f"rotation-failed={exc}; backup={backup}") from exc
    return "MT_APPROVALS=CONSISTENT BACKUP=" + str(backup)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend-id", required=True)
    parser.add_argument("--approved-sha256", required=True,
                        help="Independently verified digest of the new installed handler")
    parser.add_argument("--confirm", required=True, choices=["EXISTING-APPROVALS-ONLY"])
    args = parser.parse_args()
    if os.geteuid() != 0:
        print("MT_APPROVALS=STOP root required", file=sys.stderr)
        return 1
    try:
        print(reconcile(Path("/"), args.backend_id, args.approved_sha256))
        return 0
    except (ReconcileError, OSError) as exc:
        print("MT_APPROVALS=FAILED " + str(exc), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
