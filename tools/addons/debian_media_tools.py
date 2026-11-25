#!/usr/bin/env python3
"""Populate the normal debhelper binary package from the canonical MT.I1 stage.

Called only in an isolated Debian source build, after the existing Suite
runtime and all three Media Tools worker targets have been built. No live
installation, grant, daemon restart, separate worker build or file inventory.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "tools"))
from build_media_tools_cpp_deb import collision_preinst, stage_package_paths
from stage_media_tools_runtime import INVENTORY_REL, WORKERS, compose, digest, verify_media_tools_stage
from install_runtime_stage import load_manifest, verify_sealed_stage


def source_commit() -> str:
    """Read exact Git provenance, or an explicitly labelled recipe prefix."""
    marker = (ROOT / ".vdr-suite-source-commit").read_text(encoding="ascii").strip()
    if re.fullmatch(r"[0-9a-f]{40}", marker):
        return marker
    if marker != "$Format:%H$":
        raise ValueError("invalid source archive commit marker")
    if not (ROOT / ".git").exists():
        changelog = (ROOT / "debian/changelog").read_text(encoding="utf-8").splitlines()[0]
        version = re.match(r"vdr-suite \(([^)]+)\)", changelog)
        prefix = (re.search(r"\+g([0-9a-f]{7,39})~ubuntu[0-9.]+$", version[1])
                  if version else None)
        if prefix:
            return "git-prefix:" + prefix[1]
        raise ValueError("source archive lacks Git commit or recipe commit prefix")
    resolved = subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "--show-toplevel"],
        text=True, timeout=10,
    ).strip()
    if Path(resolved).resolve() != ROOT.resolve():
        raise ValueError("source checkout identity is not verified")
    commit = subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True, timeout=10,
    ).strip()
    if not re.fullmatch(r"[0-9a-f]{40}", commit):
        raise ValueError("invalid checkout commit")
    return commit


def build_package_payload(base_stage: Path, workers: Path,
                          package_root: Path, preinst: Path) -> dict:
    """Reuse the existing composer/inventory and populate debhelper's package root."""
    if not base_stage.is_absolute() or base_stage.is_symlink():
        raise ValueError("absolute non-symlinked Suite stage required")
    if not workers.is_absolute() or workers.is_symlink():
        raise ValueError("absolute non-symlinked worker directory required")
    if not package_root.is_absolute() or package_root.is_symlink():
        raise ValueError("absolute non-symlinked package root required")
    if not preinst.is_absolute() or preinst.is_symlink():
        raise ValueError("absolute non-symlinked preinst path required")
    verify_sealed_stage(base_stage, load_manifest(base_stage))
    source = source_commit()
    handler = workers / WORKERS[0]
    approved = digest(handler)
    if package_root.exists() and any(package_root.rglob("*")):
        raise ValueError("Media Tools binary package root is not empty")
    if preinst.exists():
        raise ValueError("generated Media Tools preinst already exists")

    # Same prepared-workdir contract used by the manual stage composer.
    # No worker is compiled, and no separate payload list is maintained.
    with tempfile.TemporaryDirectory(prefix=".debian-media-tools-", dir=base_stage.parent) as name:
        scratch = Path(name)
        prepared = scratch / "prepared"
        prepared.mkdir()
        shutil.copytree(base_stage, prepared / "stage")
        out = prepared / "build" / "addons"
        out.mkdir(parents=True)
        for worker in WORKERS:
            candidate = workers / worker
            info = candidate.lstat()
            if (not stat.S_ISREG(info.st_mode) or not info.st_mode & stat.S_IXUSR
                    or info.st_size < 4 or info.st_size > 16 * 1024 * 1024):
                raise ValueError("missing or unsafe built worker: " + worker)
            with candidate.open("rb") as handle:
                if handle.read(4) != b"\x7fELF":
                    raise ValueError("built worker is not ELF: " + worker)
            copied = out / worker
            shutil.copyfile(candidate, copied)
            copied.chmod(0o755)
        (prepared / "candidate.txt").write_text(source + "\n" + approved + "\n", encoding="ascii")
        sealed = scratch / "sealed"
        compose(prepared, sealed, approved, recipe_source=True)
        record = verify_media_tools_stage(sealed, approved, recipe_source=True)
        paths = stage_package_paths()
        if set(paths) != set(record["files"]) | {INVENTORY_REL.as_posix()}:
            raise ValueError("Debian payload differs from the sealed inventory")
        package_root.mkdir(parents=True, exist_ok=True)
        for rel in paths:
            original = sealed / rel
            target = package_root / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(original, target)
            target.chmod(stat.S_IMODE(original.stat().st_mode))
            if digest(original) != digest(target):
                raise ValueError("Debian payload differs from sealed stage: " + rel)
        preinst.write_text(collision_preinst(paths), encoding="utf-8")
        preinst.chmod(0o755)
    print("MEDIA_TOOLS_DEBIAN_STAGE=PASS files=" + str(len(paths)))
    print("MEDIA_TOOLS_SOURCE_COMMIT=" + source)
    print("MEDIA_TOOLS_HANDLER_SHA256=" + approved)
    print("MEDIA_TOOLS_WORKERS=REUSED_NO_SECOND_BUILD")
    print("LIVE_SYSTEM=UNCHANGED")
    return record


def verify_package_payload(package_root: Path) -> int:
    """Check the final debhelper tree against the sealed inventory."""
    inventory = package_root / INVENTORY_REL
    record = json.loads(inventory.read_text(encoding="utf-8"))
    expected = stage_package_paths()
    present = {p.relative_to(package_root).as_posix()
               for p in package_root.rglob("*") if p.is_file()}
    # debhelper may additionally install Debian-required copyright/changelog
    # documentation. It must not add, move, or replace Media Tools payloads.
    docs = "usr/share/doc/vdr-suite-addon-media-tools/"
    if set(expected) != {p for p in present if not p.startswith(docs)}:
        raise ValueError("Debian payload file set differs from stage inventory")
    entries = record["files"]
    if set(entries) != set(expected) - {INVENTORY_REL.as_posix()}:
        raise ValueError("Debian payload inventory differs")
    for rel in expected:
        path = package_root / rel
        if path.is_symlink() or not path.is_file():
            raise ValueError("invalid Debian payload: " + rel)
        if rel == INVENTORY_REL.as_posix():
            if stat.S_IMODE(path.stat().st_mode) != 0o644:
                raise ValueError("modified Debian inventory mode")
            continue
        entry = entries[rel]
        if (digest(path) != entry["sha256"] or
                stat.S_IMODE(path.stat().st_mode) != entry["mode"] or
                path.stat().st_size != entry["size"]):
            raise ValueError("Debian package changed sealed stage bytes or mode: " + rel)
    print("MEDIA_TOOLS_DEBIAN_PAYLOAD=PASS files=" + str(len(expected)))
    return len(expected)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-stage", type=Path)
    parser.add_argument("--workers", type=Path)
    parser.add_argument("--package-root", type=Path, required=True)
    parser.add_argument("--preinst", type=Path)
    parser.add_argument("--verify-package-root", action="store_true")
    args = parser.parse_args()
    try:
        if args.verify_package_root:
            if any((args.base_stage, args.workers, args.preinst)):
                parser.error("verification takes only --package-root")
            verify_package_payload(args.package_root)
        else:
            if not all((args.base_stage, args.workers, args.preinst)):
                parser.error("build requires --base-stage, --workers and --preinst")
            build_package_payload(args.base_stage, args.workers, args.package_root, args.preinst)
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError,
            subprocess.TimeoutExpired) as exc:
        print("MEDIA_TOOLS_DEBIAN_STAGE=FAIL " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
