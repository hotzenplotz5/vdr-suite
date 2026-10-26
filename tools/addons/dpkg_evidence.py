#!/usr/bin/env python3
"""Offline dpkg ledger consistency for reviewed VDR-Suite add-on metadata.

This is a read-only *diagnostic*. dpkg's local status/list/MD5 database is
mutable host state, NOT signed provenance or trusted handler authorization.
Never map its result directly to RuntimeEvidence.packageTrusted.
"""
from __future__ import annotations

import hashlib
import json
import re
import stat
from pathlib import Path

from addon_contract import FILES
from stage_media_tools_runtime import INVENTORY_REL, payload_roles

STATUS_LIMIT = 32 * 1024 * 1024
LIST_LIMIT = 1024 * 1024
MD5_LIMIT = 1024 * 1024
HEX_MD5 = re.compile(r"[0-9a-f]{32}\Z")


class LedgerError(ValueError):
    def __init__(self, reason: str):
        super().__init__(reason)
        self.reason = reason


def _regular_data(root: Path, relative: str, maximum: int) -> bytes:
    """Refuse symlink traversal through the injected filesystem root."""
    path = root / relative
    cursor = root
    try:
        chain = (root, *(root / Path(*Path(relative).parts[:n])
                         for n in range(1, len(Path(relative).parts) + 1)))
        for component in chain:
            mode = component.lstat().st_mode
            if stat.S_ISLNK(mode):
                raise LedgerError("unsafe-ledger-path")
            if component != path and not stat.S_ISDIR(mode):
                raise LedgerError("unsafe-ledger-path")
        if not stat.S_ISREG(path.lstat().st_mode):
            raise LedgerError("unsafe-ledger-path")
        if path.stat().st_size > maximum:
            raise LedgerError("oversized-ledger")
        with path.open("rb") as handle:
            data = handle.read(maximum + 1)
        if not data or len(data) > maximum:
            raise LedgerError("missing-or-oversized-ledger")
        return data
    except (OSError, ValueError) as exc:
        if isinstance(exc, LedgerError):
            raise
        raise LedgerError("missing-or-unreadable-ledger") from exc


def _record_for_package(text: str, package: str) -> dict[str, str]:
    selected = []
    for block in text.split("\n\n"):
        if not block.strip():
            continue
        fields = {}
        for line in block.splitlines():
            if line.startswith((" ", "\t")):
                continue  # Debian control field continuation
            if ":" not in line:
                raise LedgerError("invalid-dpkg-status")
            name, value = line.split(":", 1)
            if not name or name in fields:
                raise LedgerError("invalid-dpkg-status")
            fields[name] = value.strip()
        if fields.get("Package") == package:
            selected.append(fields)
    if len(selected) != 1:
        raise LedgerError("package-absent-or-duplicate")
    return selected[0]


def _file_list(text: str) -> set[str]:
    paths = text.splitlines()
    if len(paths) != len(set(paths)):
        raise LedgerError("duplicate-dpkg-file")
    if any(not p.startswith("/") or ".." in Path(p).parts
           or "\x00" in p or "\\" in p for p in paths):
        raise LedgerError("unsafe-dpkg-file-list")
    return set(paths)


def _checksums(text: str) -> dict[str, str]:
    result = {}
    for line in text.splitlines():
        checksum, separator, path = line.partition("  ")
        if separator != "  " or not HEX_MD5.fullmatch(checksum):
            raise LedgerError("invalid-dpkg-checksums")
        if (path.startswith("/") or not path
                or ".." in Path(path).parts or "." in Path(path).parts
                or "\\" in path or "\x00" in path or path in result):
            raise LedgerError("invalid-dpkg-checksums")
        result[path] = checksum
    return result



def _verify_sealed_stage_record(root: Path, version: str, package: str,
                                owned: set[str], recorded: dict[str, str]) -> None:
    """Stage-1 dpkg diagnostic: exact eight-file payload, never package trust."""
    inventory_rel = INVENTORY_REL.as_posix()
    try:
        raw = _regular_data(root, inventory_rel, 65536)
        inventory = json.loads(raw.decode("utf-8"))
        if not isinstance(inventory, dict) or set(inventory) != {
            "schema", "module", "package", "version", "prepared_source_sha",
            "approved_handler_sha256", "files"
        } or inventory["schema"] != 1 or inventory["module"] != "rectools" or (
            inventory["package"] != package):
            raise LedgerError("stage-inventory-invalid")
        source_sha = inventory["prepared_source_sha"]
        if not isinstance(source_sha, str) or not re.fullmatch(r"[0-9a-f]{40}", source_sha):
            raise LedgerError("stage-inventory-invalid")
        if (version != inventory["version"] + "~stage1+git" + source_sha[:12] or
                not re.fullmatch(r"[0-9a-f]{64}",
                                 str(inventory["approved_handler_sha256"]))):
            raise LedgerError("stage-inventory-mismatch")
        roles = payload_roles()
        if not isinstance(inventory["files"], dict) or set(inventory["files"]) != set(roles):
            raise LedgerError("stage-inventory-invalid")
        required = set(roles) | {inventory_rel}
        if (not required.issubset({p.removeprefix("/") for p in owned}) or
                not required.issubset(recorded)):
            raise LedgerError("stage-payload-not-dpkg-owned")
        if hashlib.md5(raw).hexdigest() != recorded[inventory_rel]:
            raise LedgerError("stage-payload-checksum-mismatch")
        for relative, role in roles.items():
            details = inventory["files"][relative]
            if (not isinstance(details, dict) or
                    set(details) != {"role", "sha256", "size", "mode"} or
                    details["role"] != role or
                    not isinstance(details["size"], int) or
                    not 0 < details["size"] <= 128 * 1024 * 1024 or
                    not isinstance(details["mode"], int) or
                    details["mode"] not in (0o644, 0o755) or
                    not isinstance(details["sha256"], str) or
                    not re.fullmatch(r"[0-9a-f]{64}", details["sha256"])):
                raise LedgerError("stage-inventory-invalid")
            body = _regular_data(root, relative, 128 * 1024 * 1024)
            if (len(body) != details["size"] or
                    stat.S_IMODE((root / relative).lstat().st_mode) != details["mode"] or
                    hashlib.sha256(body).hexdigest() != details["sha256"] or
                    hashlib.md5(body).hexdigest() != recorded[relative]):
                raise LedgerError("stage-payload-checksum-mismatch")
        worker = "usr/libexec/vdr-suite/addons/rectools/vdr-suite-media-import"
        if inventory["files"][worker]["sha256"] != inventory["approved_handler_sha256"]:
            raise LedgerError("stage-inventory-mismatch")
    except (OSError, UnicodeError, KeyError, TypeError, ValueError) as exc:
        if isinstance(exc, LedgerError):
            raise
        raise LedgerError("stage-inventory-invalid") from exc

def examine(root: Path, module: str, package: str, manifest_version: str) -> dict:
    """Compare package-owned metadata to one dpkg database in the SAME root.

    No subprocess or network, no package load/install, no trusted flag.
    Even 'consistent' is not an authorization/provenance attestation.
    """
    try:
        if not root.is_absolute() or ".." in root.parts or "." in root.parts:
            raise LedgerError("unsafe-root")
        if not re.fullmatch(r"[a-z][a-z0-9-]{1,100}", package):
            raise LedgerError("unsafe-package")
        if module not in ("rectools", "image", "music"):
            raise LedgerError("unregistered-module")
        if not re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+", manifest_version):
            raise LedgerError("invalid-version")
        info = "var/lib/dpkg/info/" + package
        status = _record_for_package(
            _regular_data(root, "var/lib/dpkg/status", STATUS_LIMIT).decode("utf-8"),
            package,
        )
        if status.get("Status") != "install ok installed":
            raise LedgerError("package-not-installed")
        suffix = "~cpppreview1" if module == "rectools" else "~scaffold1"
        valid_versions = {manifest_version + "~scaffold1",
                          manifest_version + suffix}
        package_version = status.get("Version", "")
        stage_package = module == "rectools" and bool(re.fullmatch(
            re.escape(manifest_version) + r"~stage1\+git[0-9a-f]{12}",
            package_version))
        if package_version not in valid_versions and not stage_package:
            raise LedgerError("package-version-mismatch")
        if status.get("Architecture") not in ("all", "amd64", "arm64", "armhf", "i386"):
            raise LedgerError("unsupported-architecture")
        if status["Version"].endswith("~scaffold1") and status["Architecture"] != "all":
            raise LedgerError("package-architecture-mismatch")
        if (status["Version"].endswith("~cpppreview1") or stage_package) and (
                status["Architecture"] == "all"):
            raise LedgerError("package-architecture-mismatch")

        owned = _file_list(
            _regular_data(root, info + ".list", LIST_LIMIT).decode("utf-8"))
        recorded = _checksums(
            _regular_data(root, info + ".md5sums", MD5_LIMIT).decode("utf-8"))
        prefix = "usr/share/vdr-suite/addons/" + module + "/"
        for filename in FILES:
            relative = prefix + filename
            if "/" + relative not in owned or relative not in recorded:
                raise LedgerError("metadata-not-dpkg-owned")
            actual = hashlib.md5(_regular_data(root, relative, 65536)).hexdigest()
            if actual != recorded[relative]:
                raise LedgerError("metadata-checksum-mismatch")
        if stage_package:
            _verify_sealed_stage_record(root, package_version, package, owned, recorded)
        return {"consistent": True, "reason": "dpkg-ledger-consistent"}
    except (LedgerError, OSError, UnicodeError, ValueError) as exc:
        reason = exc.reason if isinstance(exc, LedgerError) else "invalid-dpkg-ledger"
        return {"consistent": False, "reason": reason}
