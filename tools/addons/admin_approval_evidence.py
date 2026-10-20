#!/usr/bin/env python3
"""Read-only local administrator-approval record diagnostics.

A root-managed approval file is independent of installed addon.json and
dpkg. This probe never produces an execution permit or packageTrusted.
The future privileged installer/Agent must own issuance, revocation and
descriptor-pinned dispatch validation.
"""
from __future__ import annotations

import json
import re
import stat
from pathlib import Path

from manual_handler_evidence import examine as examine_binary
from addon_contract import ContractError, validate_directory

APPROVAL_PATH = "etc/vdr-suite/addon-approvals/rectools.json"
BINARY_PATH = "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
EXPECTED_KEYS = frozenset(("schemaVersion", "id", "package", "version", "sha256", "origin"))
HEX = re.compile(r"[0-9a-f]{64}\Z")
VERSION = re.compile(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\Z")


def examine(root: Path, module: str) -> dict:
    def denied(reason: str) -> dict:
        return {"consistent": False, "reason": reason}
    if module != "rectools":
        return denied("no-reviewed-approval")
    if not root.is_absolute() or "." in root.parts or ".." in root.parts:
        return denied("unsafe-root")
    path = root
    try:
        for component in APPROVAL_PATH.split("/"):
            parent = path.lstat()
            if (not stat.S_ISDIR(parent.st_mode) or parent.st_uid != 0 or
                    parent.st_mode & 0o022):
                return denied("unsafe-approval-parent")
            path = path / component
        meta = path.lstat()
        if (not stat.S_ISREG(meta.st_mode) or meta.st_uid != 0 or
                meta.st_mode & 0o022 or not 0 < meta.st_size <= 4096):
            return denied("unsafe-approval-record")
        import os
        fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0))
        with os.fdopen(fd, "rb") as file:
            info = os.fstat(file.fileno())
            if (info.st_dev, info.st_ino, info.st_mtime_ns, info.st_size) != (
                    meta.st_dev, meta.st_ino, meta.st_mtime_ns, meta.st_size):
                return denied("approval-record-changed")
            raw = file.read(4097)
            if len(raw) != meta.st_size:
                return denied("approval-record-changed")
            if len(raw) > 4096:
                return denied("unsafe-approval-record")
            after = os.fstat(file.fileno())
            if (after.st_ctime_ns, after.st_mtime_ns, after.st_size) != (
                    info.st_ctime_ns, info.st_mtime_ns, info.st_size):
                return denied("approval-record-changed")
        def unique(pairs):
            data = {}
            for key, value in pairs:
                if key in data:
                    raise ValueError("duplicate JSON key")
                data[key] = value
            return data
        record = json.loads(raw.decode("utf-8"), object_pairs_hook=unique)
        if not isinstance(record, dict) or set(record) != EXPECTED_KEYS:
            return denied("invalid-approval-schema")
        if type(record["schemaVersion"]) is not int or record["schemaVersion"] != 1:
            return denied("invalid-approval-schema")
        if record["id"] != "rectools" or record["package"] != "vdr-suite-addon-media-tools":
            return denied("invalid-approval-identity")
        if not isinstance(record["version"], str) or not VERSION.fullmatch(record["version"]):
            return denied("invalid-approval-version")
        if not isinstance(record["sha256"], str) or not HEX.fullmatch(record["sha256"]):
            return denied("invalid-approval-digest")
        if record["origin"] not in ("manual", "debian"):
            return denied("invalid-approval-origin")
        manifest_path = root / "usr/share/vdr-suite/addons/rectools"
        try:
            manifest = validate_directory(manifest_path, "rectools")
        except (ContractError, OSError, ValueError, UnicodeError):
            return denied("invalid-installed-metadata")
        if manifest["version"] != record["version"] or manifest["package"] != record["package"]:
            return denied("approval-metadata-mismatch")
        binary = examine_binary(root, module, record["sha256"])
        if not binary["consistent"]:
            return denied("approved-binary-" + binary["reason"])
        # This proves agreement with local root-owned administrative state,
        # not an authenticated release signature or runtime dispatch lease.
        return {"consistent": True, "reason": "root-owned-approval-and-binary-match"}
    except (OSError, ValueError, UnicodeError, TypeError):
        return denied("missing-or-invalid-approval")


if __name__ == "__main__":
    raise SystemExit("diagnostic module only; use the isolated registry tests")
