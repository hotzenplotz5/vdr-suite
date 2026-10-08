#!/usr/bin/env python3
"""Fail-closed validation and source-package staging for inert add-ons.

No runtime code loading, installation to live root or media execution.
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
import tempfile
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[2]
FILES = ("addon.json", "README.md", "AGENTS.md")
FIELDS = frozenset((
    "schemaVersion", "id", "version", "package", "state",
    "displayName", "description", "entrypoints", "capabilities",
    "permissions", "dependencies",
))
NAME = re.compile(r"[a-z][a-z0-9]*(?:-[a-z0-9]+)*\Z")
VERSION = re.compile(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\Z")


class ContractError(ValueError):
    pass


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ContractError("duplicate JSON key: " + key)
        result[key] = value
    return result


def safe_source(path):
    if path.is_symlink() or not path.is_file():
        raise ContractError("missing/symlinked source: " + str(path))
    raw = path.read_bytes()
    if not raw or len(raw) > 65536:
        raise ContractError("invalid file size: " + str(path))
    return raw


def validate_directory(module, name):
    """Validate either a source module or a staged installed module.

    This is syntax/metadata validation, NOT executable/package provenance.
    """
    if not isinstance(name, str) or not NAME.fullmatch(name):
        raise ContractError("invalid module directory name")
    if module.is_symlink() or not module.is_dir():
        raise ContractError("missing/symlinked module directory: " + name)
    raw = safe_source(module / FILES[0])
    for filename in FILES[1:]:
        safe_source(module / filename)
    try:
        obj = json.loads(raw.decode("utf-8"), object_pairs_hook=unique_keys)
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise ContractError("invalid JSON: " + name) from exc
    if not isinstance(obj, dict) or set(obj) != FIELDS:
        raise ContractError("schema-v1 fields mismatch: " + name)
    if type(obj["schemaVersion"]) is not int or obj["schemaVersion"] != 1:
        raise ContractError("unsupported schema version: " + name)
    if obj["id"] != name:
        raise ContractError("module ID mismatches directory: " + name)
    if not isinstance(obj["version"], str) or not VERSION.fullmatch(obj["version"]):
        raise ContractError("invalid version: " + name)
    package = obj["package"]
    if not isinstance(package, str) or not NAME.fullmatch(package) or not package.startswith("vdr-suite-addon-"):
        raise ContractError("invalid package name: " + name)
    if obj["state"] != "scaffold":
        raise ContractError("only disabled scaffold is currently supported: " + name)
    for field in ("displayName", "description"):
        value = obj[field]
        if not isinstance(value, str) or not 1 <= len(value.strip()) <= 300 or value != value.strip():
            raise ContractError("invalid text field " + field + ": " + name)
        if any(ord(ch) < 32 for ch in value):
            raise ContractError("control character in " + field + ": " + name)
    for field in ("entrypoints", "capabilities", "permissions", "dependencies"):
        if obj[field] != []:
            raise ContractError("scaffold cannot activate " + field + ": " + name)
    return obj


def validate_module(root, name):
    return validate_directory(root / "modules" / name, name)


def validate_all(root=ROOT):
    directory = root / "modules"
    if directory.is_symlink() or not directory.is_dir():
        raise ContractError("missing modules source root")
    names = sorted(item.name for item in directory.iterdir() if item.is_dir() or item.is_symlink())
    if not names:
        raise ContractError("no module source directories")
    packages, modules = set(), {}
    for name in names:
        manifest = validate_module(root, name)
        if manifest["package"] in packages:
            raise ContractError("duplicate package name: " + manifest["package"])
        packages.add(manifest["package"])
        modules[name] = manifest
    return modules


def validate_stage_root(destdir, prefix):
    if not destdir:
        raise ContractError("DESTDIR required; refusing live install")
    path = Path(destdir)
    if not path.is_absolute() or any(p in (".", "..") for p in path.parts):
        raise ContractError("DESTDIR must be absolute without traversal")
    if str(path) in ("/", "/usr", "/usr/local", "/etc", "/var", "/home", "/tmp"):
        raise ContractError("DESTDIR must be isolated; live system root rejected")
    if not path.is_dir() or any(p.is_symlink() for p in (path, *path.parents)):
        raise ContractError("DESTDIR must be an existing, non-symlinked directory")
    if not prefix or not prefix.startswith("/") or prefix.startswith("//") or "\\" in prefix:
        raise ContractError("invalid PREFIX")
    fragments = prefix.split("/")[1:]
    if not fragments or any(p in ("", ".", "..") for p in fragments):
        raise ContractError("unsafe PREFIX")
    return path, PurePosixPath(prefix).parts[1:]


def stage(root, module, destdir, prefix="/usr"):
    manifests = validate_all(root)
    if module not in manifests:
        raise ContractError("unknown add-on module")
    directory, fragments = validate_stage_root(destdir, prefix)
    parent = directory.joinpath(*fragments, "share", "vdr-suite", "addons")
    target = parent / module
    if target.exists() or target.is_symlink():
        raise ContractError("refusing to overwrite existing staged payload")
    cursor = directory
    for name in (*fragments, "share", "vdr-suite", "addons", module):
        cursor = cursor / name
        if cursor.is_symlink():
            raise ContractError("symlink in staged destination")
    parent.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix="." + module + ".", dir=parent))
    try:
        for filename in FILES:
            destination = tmp / filename
            shutil.copyfile(root / "modules" / module / filename, destination)
            destination.chmod(0o644)
        tmp.rename(target)
    finally:
        if tmp.exists():
            shutil.rmtree(tmp)
    return target


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("check", "stage"))
    parser.add_argument("--module", default="")
    parser.add_argument("--destdir", default="")
    parser.add_argument("--prefix", default="/usr")
    args = parser.parse_args(argv)
    try:
        manifests = validate_all()
        if args.command == "check":
            print("ADDON_CONTRACT=PASS modules=" + ",".join(manifests))
        else:
            result = stage(ROOT, args.module, args.destdir, args.prefix)
            print("ADDON_STAGE=PASS module=" + args.module + " path=" + str(result))
    except (ContractError, OSError) as exc:
        print("ADDON_CONTRACT=FAIL " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
