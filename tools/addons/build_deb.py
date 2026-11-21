#!/usr/bin/env python3
"""Build a nonfunctional, metadata-only Debian add-on package from module sources.

The explicit maintainer must be supplied by the package builder; no install,
apt, network, root operation or service start is performed.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from addon_contract import (
    ROOT, FILES, ContractError, stage, validate_all, validate_stage_root,
)

MAINTAINER = re.compile(r"[^<>\r\n]{1,100} <[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}>\Z")
BUDGET_BYTES = 16 * 1024 * 1024


def validate_maintainer(value):
    if not isinstance(value, str) or value != value.strip() or not MAINTAINER.fullmatch(value):
        raise ContractError("real maintainer identity required: Name <email@example.org>")
    return value


def build(root: Path, module: str, output_dir: str, maintainer: str) -> Path:
    manifests = validate_all(root)
    if module not in manifests:
        raise ContractError("unknown add-on module")
    manifest = manifests[module]
    maintainer = validate_maintainer(maintainer)
    output, _ = validate_stage_root(output_dir, "/usr")
    if not shutil.which("dpkg-deb"):
        raise ContractError("dpkg-deb not installed; refusing fake package result")

    usage = shutil.disk_usage(output)
    reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
    print("RESOURCE_FS=addon-output total=" + str(usage.total)
          + " free=" + str(usage.free) + " budget=" + str(BUDGET_BYTES)
          + " reserve=" + str(reserve))
    if usage.free < reserve + BUDGET_BYTES:
        raise ContractError("RESOURCE_PREFLIGHT=FAIL insufficient isolated package space")
    print("RESOURCE_PREFLIGHT=PASS")

    version = manifest["version"] + "~scaffold1"
    target = output / (manifest["package"] + "_" + version + "_all.deb")
    if target.exists() or target.is_symlink():
        raise ContractError("refusing to overwrite existing .deb")
    with tempfile.TemporaryDirectory(prefix="vdr-suite-addon-build-", dir=output) as temporary:
        base = Path(temporary)
        stage_root = base / "package-root"
        stage_root.mkdir()
        stage(root, module, str(stage_root))
        control = stage_root / "DEBIAN"
        control.mkdir(mode=0o755)
        content = (
            "Package: " + manifest["package"] + "\n"
            "Version: " + version + "\n"
            "Architecture: all\n"
            "Section: misc\n"
            "Priority: optional\n"
            "Maintainer: " + maintainer + "\n"
            "Description: Inactive VDR-Suite " + manifest["displayName"] + " add-on scaffold\n"
            " Source-only metadata and module documentation. No functional add-on,\n"
            " executable, web UI, permission, service or worker is installed.\n"
        )
        (control / "control").write_text(content, encoding="utf-8")
        (control / "control").chmod(0o644)
        temporary_deb = base / "package.deb"
        try:
            subprocess.run(
                ["dpkg-deb", "--build", "--root-owner-group", str(stage_root), str(temporary_deb)],
                check=True, capture_output=True, text=True, timeout=60,
            )
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
            raise ContractError("dpkg-deb failed; no output package published") from exc
        if not temporary_deb.is_file() or temporary_deb.stat().st_size == 0:
            raise ContractError("dpkg-deb returned no nonempty package")
        # Atomic no-clobber publication on the same filesystem.
        os.link(temporary_deb, target)

    post = shutil.disk_usage(output)
    if post.free < reserve:
        target.unlink(missing_ok=True)
        raise ContractError("RESOURCE_POSTCHECK=FAIL reserve not preserved")
    print("RESOURCE_POSTCHECK=PASS")
    return target


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--maintainer", default="")
    args = parser.parse_args(argv)
    try:
        result = build(ROOT, args.module, args.output_dir, args.maintainer)
        print("ADDON_DEB=PASS path=" + str(result))
    except (OSError, ContractError) as exc:
        print("ADDON_DEB=FAIL " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
