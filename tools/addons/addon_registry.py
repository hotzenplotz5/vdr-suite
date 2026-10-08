#!/usr/bin/env python3
"""Read-only, fail-closed installed add-on discovery and activation evaluation.

No daemon route, package installation, process launch or writable admin API.
The status is deliberately *not* an authorization decision for actor requests.
"""
from __future__ import annotations

import json
import os
import stat
import sys
from pathlib import Path

from addon_contract import (
    FILES, FIELDS, NAME, VERSION, ContractError, unique_keys,
)

# Package-owned metadata and separately administrator-owned desired state.
INSTALLED_ROOT = Path("/usr/share/vdr-suite/addons")
POLICY_ROOT = Path("/etc/vdr-suite/addons-enabled.d")
MAX_MANIFEST_BYTES = 65536
MAX_POLICY_BYTES = 4096
MAX_MODULES = 128


def _regular_bytes(path: Path, limit: int) -> bytes:
    info = path.lstat()
    if not stat.S_ISREG(info.st_mode) or info.st_size < 1 or info.st_size > limit:
        raise ContractError("expected bounded regular file")
    with path.open("rb") as stream:
        data = stream.read(limit + 1)
    if not data or len(data) > limit:
        raise ContractError("file changed or exceeds size limit")
    return data


def _json_object(path: Path, limit: int) -> dict:
    try:
        value = json.loads(_regular_bytes(path, limit).decode("utf-8"),
                           object_pairs_hook=unique_keys)
    except (UnicodeError, ValueError) as exc:
        raise ContractError("invalid UTF-8/JSON object") from exc
    if not isinstance(value, dict):
        raise ContractError("JSON object required")
    return value


def _trusted(path: Path, required_uid: int = 0, stop_at: Path | None = None) -> bool:
    """Require safe owner/modes, no symlinks, including the policy/module root.

    Production uses the entire absolute ancestor chain. Isolated tests limit
    the chain to their explicitly injected root because /tmp is sticky-writable.
    """
    try:
        if stop_at is not None and path != stop_at and stop_at not in path.parents:
            return False
        for component in (path, *path.parents):
            info = component.lstat()
            if stat.S_ISLNK(info.st_mode) or info.st_uid != required_uid:
                return False
            if info.st_mode & (stat.S_IWGRP | stat.S_IWOTH):
                return False
            if stop_at is not None and component == stop_at:
                return True
        return stop_at is None
    except (OSError, ValueError):
        return False


def _manifest(folder: Path, name: str) -> dict:
    if not NAME.fullmatch(name):
        raise ContractError("invalid add-on identifier")
    if not folder.is_dir() or folder.is_symlink():
        raise ContractError("invalid add-on directory")
    for filename in FILES[1:]:
        _regular_bytes(folder / filename, MAX_MANIFEST_BYTES)
    manifest = _json_object(folder / "addon.json", MAX_MANIFEST_BYTES)
    if set(manifest) != FIELDS:
        raise ContractError("unsupported manifest fields")
    if type(manifest["schemaVersion"]) is not int or manifest["schemaVersion"] != 1:
        raise ContractError("unsupported schema version")
    if manifest["id"] != name or manifest["state"] != "scaffold":
        raise ContractError("invalid module identity or unsupported lifecycle state")
    if not isinstance(manifest["version"], str) or not VERSION.fullmatch(manifest["version"]):
        raise ContractError("invalid module version")
    package = manifest["package"]
    if not isinstance(package, str) or not NAME.fullmatch(package) or not package.startswith("vdr-suite-addon-"):
        raise ContractError("invalid package identity")
    for field in ("displayName", "description"):
        value = manifest[field]
        if not isinstance(value, str) or not 1 <= len(value.strip()) <= 300:
            raise ContractError("invalid metadata text")
    for field in ("entrypoints", "capabilities", "permissions", "dependencies"):
        if manifest[field] != []:
            raise ContractError("scaffold cannot declare runtime functionality")
    return manifest


def _requested(policy_root: Path, name: str, required_uid: int) -> bool:
    policy = policy_root / (name + ".json")
    try:
        policy.lstat()
    except FileNotFoundError:
        return False  # no opt-in
    if not _trusted(policy, required_uid,
                    None if policy_root == POLICY_ROOT else policy_root):
        raise ContractError("policy ownership or permissions are not trusted")
    value = _json_object(policy, MAX_POLICY_BYTES)
    if (set(value) != {"schemaVersion", "module", "enabled"} or
            type(value["schemaVersion"]) is not int or value["schemaVersion"] != 1 or
            value["module"] != name or type(value["enabled"]) is not bool):
        raise ContractError("invalid activation policy schema")
    return value["enabled"]


def discover(installed_root: Path = INSTALLED_ROOT,
             policy_root: Path = POLICY_ROOT,
             required_uid: int = 0) -> dict:
    """Report installed source metadata, not active operations.

    The root is fixed by the CLI. Tests inject isolated directories. Every
    error fails that module closed; invalid module names cannot become routes.
    """
    entries = []
    try:
        installed_root.lstat()
    except FileNotFoundError:
        return {"schemaVersion": 1, "modules": [], "enabledCapabilities": []}
    if not installed_root.is_dir() or not _trusted(
            installed_root, required_uid,
            None if installed_root == INSTALLED_ROOT else installed_root):
        raise ContractError("untrusted installed add-on directory")
    try:
        folders = sorted(installed_root.iterdir())
    except OSError as exc:
        raise ContractError("cannot enumerate installed modules") from exc
    if len(folders) > MAX_MODULES:
        raise ContractError("too many entries in installed add-on directory")
    for folder in folders:
        name = folder.name
        if not NAME.fullmatch(name):
            # Invalid names cannot register a module ID, and are ignored.
            continue
        item = {
            "id": name,
            "installed": False,
            "version": None,
            "package": None,
            "requestedEnabled": False,
            "effectiveEnabled": False,
            "capabilities": [],
            "reason": "invalid_manifest",
        }
        try:
            if not _trusted(folder, required_uid, installed_root):
                raise ContractError("untrusted module ownership")
            for filename in FILES:
                if not _trusted(folder / filename, required_uid, installed_root):
                    raise ContractError("untrusted module metadata ownership")
            manifest = _manifest(folder, name)
            item["installed"] = True
            item["version"] = manifest["version"]
            item["package"] = manifest["package"]
            # A bad or untrusted policy blocks the module even if requested.
            item["requestedEnabled"] = _requested(policy_root, name, required_uid)
            item["reason"] = "scaffold_not_executable"
        except (ContractError, OSError):
            item["reason"] = "invalid_or_untrusted"
            item["requestedEnabled"] = False
        entries.append(item)
    # Duplicate package identity invalidates *every* claimant, not merely
    # whichever name happens to sort second.
    package_counts = {}
    for item in entries:
        if item["installed"]:
            package_counts[item["package"]] = package_counts.get(item["package"], 0) + 1
    for item in entries:
        if item["installed"] and package_counts[item["package"]] > 1:
            item["installed"] = False
            item["requestedEnabled"] = False
            item["reason"] = "invalid_or_untrusted"
    return {"schemaVersion": 1, "modules": entries, "enabledCapabilities": []}


def main() -> int:
    if sys.argv[1:] not in ([], ["--json"]):
        print("Usage: python3 tools/addons/addon_registry.py [--json]", file=sys.stderr)
        return 2
    try:
        result = discover()
        print(json.dumps(result, indent=2, sort_keys=True))
    except (ContractError, OSError) as exc:
        print("ADDON_REGISTRY=FAIL " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
