#!/usr/bin/env python3
"""Offline dpkg ledger consistency for reviewed VDR-Suite add-on metadata.

This is a read-only *diagnostic*. dpkg's local status/list/MD5 database is
mutable host state, NOT signed provenance or trusted handler authorization.
Never map its result directly to RuntimeEvidence.packageTrusted.
"""
from __future__ import annotations

import hashlib
import re
import stat
from pathlib import Path

from addon_contract import FILES

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
        if status.get("Version") not in valid_versions:
            raise LedgerError("package-version-mismatch")
        if status.get("Architecture") not in ("all", "amd64", "arm64", "armhf", "i386"):
            raise LedgerError("unsupported-architecture")
        if status["Version"].endswith("~scaffold1") and status["Architecture"] != "all":
            raise LedgerError("package-architecture-mismatch")
        if status["Version"].endswith("~cpppreview1") and status["Architecture"] == "all":
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
        return {"consistent": True, "reason": "dpkg-ledger-consistent"}
    except (LedgerError, OSError, UnicodeError, ValueError) as exc:
        reason = exc.reason if isinstance(exc, LedgerError) else "invalid-dpkg-ledger"
        return {"consistent": False, "reason": reason}
