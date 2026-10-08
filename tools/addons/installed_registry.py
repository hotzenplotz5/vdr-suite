#!/usr/bin/env python3
"""Read-only, fail-closed inventory of installed VDR-Suite add-on metadata.

Not an activation service or public API. Nothing in installed addon.json is
trusted to grant actor rights, load code, register HTTP routes or start jobs.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from addon_contract import ContractError, validate_directory

# Core-owned review boundary. A package cannot add itself to this table merely
# by dropping a manifest under /usr/share/vdr-suite/addons.
SUPPORTED_PACKAGES = {
    "rectools": "vdr-suite-addon-media-tools",
    "image": "vdr-suite-addon-image",
    "music": "vdr-suite-addon-music",
    "tvscraper": "vdr-suite-addon-tvscraper",
}

# The current registry contract has no approved execution adapters. A new
# module must pass separately reviewed core authority/handler registration
# before it can ever be eligible for activation.
REGISTERED_HANDLERS = frozenset()

def _check_root(root: Path) -> Path:
    if not root.is_absolute() or ".." in root.parts or "." in root.parts:
        raise ContractError("inventory root must be an absolute normalized directory")
    if not root.is_dir() or any(part.is_symlink() for part in (root, *root.parents)):
        raise ContractError("inventory root must be a real directory, not a symlink")
    return root


def _directory(root: Path) -> Path:
    cursor = _check_root(root)
    for part in ("usr", "share", "vdr-suite", "addons"):
        cursor = cursor / part
        if cursor.is_symlink():
            raise ContractError("symlinked installed add-on directory is prohibited")
        if cursor.exists() and not cursor.is_dir():
            raise ContractError("non-directory installed add-on path component")
    return cursor


def inspect(root: Path) -> dict:
    """Inventory is descriptive only; it never implies 'enabled'."""
    catalog = _directory(root)
    installed = {}
    if catalog.is_dir():
        for item in sorted(catalog.iterdir(), key=lambda p: p.name):
            # Unknown and non-directory entries do not receive capabilities.
            if item.name not in SUPPORTED_PACKAGES:
                installed[item.name] = {
                    "id": item.name,
                    "package": None,
                    "installed": False,
                    "state": "rejected",
                    "reason": "unregistered-package",
                    "active": False,
                    "capabilities": [],
                }
                continue
            if item.is_symlink() or not item.is_dir():
                installed[item.name] = {
                    "id": item.name,
                    "package": SUPPORTED_PACKAGES[item.name],
                    "installed": False,
                    "state": "rejected",
                    "reason": "invalid-package-directory",
                    "active": False,
                    "capabilities": [],
                }
                continue
            try:
                data = validate_directory(item, item.name)
                if data["package"] != SUPPORTED_PACKAGES[item.name]:
                    raise ContractError("manifest package mismatch")
                installed[item.name] = {
                    "id": item.name,
                    "package": data["package"],
                    "installed": True,
                    "version": data["version"],
                    "state": "installed-disabled",
                    "reason": "no-authorized-runtime-handler",
                    "active": False,
                    "capabilities": [],
                }
            except (ContractError, OSError, UnicodeError, ValueError):
                installed[item.name] = {
                    "id": item.name,
                    "package": SUPPORTED_PACKAGES[item.name],
                    "installed": False,
                    "state": "rejected",
                    "reason": "invalid-package-manifest",
                    "active": False,
                    "capabilities": [],
                }
    for name, package in SUPPORTED_PACKAGES.items():
        installed.setdefault(name, {
            "id": name,
            "package": package,
            "installed": False,
            "state": "not-installed",
            "reason": "missing-package",
            "active": False,
            "capabilities": [],
        })
    return {"registrySchemaVersion": 1, "modules": [installed[n] for n in sorted(installed)]}


def plan_enable(root: Path, name: str) -> dict:
    """No mutation. An enable request without a core handler always fails."""
    data = inspect(root)
    record = next((m for m in data["modules"] if m["id"] == name), None)
    if record is None or name not in SUPPORTED_PACKAGES:
        reason = "unregistered-package"
    elif not record["installed"]:
        reason = record["reason"]
    elif name not in REGISTERED_HANDLERS:
        reason = "no-authorized-runtime-handler"
    else:
        # Future code must add explicit actor, dependency, compatibility,
        # backend and administrative state gates before making this reachable.
        reason = "activation-not-implemented"
    return {
        "id": name,
        "decision": "denied",
        "reason": reason,
        "active": False,
        "sideEffects": False,
    }


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("inventory", "plan-enable"))
    parser.add_argument("--root", required=True, help="Explicit filesystem root, e.g. isolated DESTDIR")
    parser.add_argument("--module", default="")
    args = parser.parse_args(argv)
    try:
        root = Path(args.root)
        if args.action == "inventory":
            result = inspect(root)
        else:
            if not args.module:
                raise ContractError("--module is required for plan-enable")
            result = plan_enable(root, args.module)
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        return 0
    except (ContractError, OSError) as exc:
        print("ADDON_REGISTRY=FAIL " + str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
