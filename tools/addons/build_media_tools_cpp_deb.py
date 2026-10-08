#!/usr/bin/env python3
"""Package the separately compiled Media Tools C++ preview without installing it.

Unlike the metadata-only ~scaffold1 packages, this owns one offline CLI under
private libexec; the Suite runtime never discovers or activates the binary.
"""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from addon_contract import ROOT, ContractError, stage, validate_all, validate_stage_root
from build_deb import validate_maintainer

EXTRA_BUDGET = 64 * 1024 * 1024


def build(root: Path, executable: str, output_dir: str, maintainer: str) -> Path:
    manifest = validate_all(root)["rectools"]
    maintainer = validate_maintainer(maintainer)
    output, _ = validate_stage_root(output_dir, "/usr")
    binary = Path(executable)
    if (not binary.is_absolute() or binary.name != "vdr-suite-media-import"
            or binary.is_symlink() or not binary.is_file()
            or not os.access(binary, os.X_OK)
            or binary.stat().st_size < 4 or binary.stat().st_size > 16 * 1024 * 1024):
        raise ContractError("expected executable, regular, source-built media-import binary")
    with binary.open("rb") as handle:
        if handle.read(4) != b"\x7fELF":
            raise ContractError("media-import executable is not ELF")
    if not shutil.which("dpkg-deb") or not shutil.which("dpkg"):
        raise ContractError("dpkg and dpkg-deb are required")
    try:
        arch = subprocess.check_output(["dpkg", "--print-architecture"],
                                       text=True, timeout=5).strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        raise ContractError("cannot identify Debian architecture") from exc
    if not arch.isascii() or not arch.replace("-", "").isalnum() or not arch:
        raise ContractError("unsafe Debian architecture")

    usage = shutil.disk_usage(output)
    reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
    print(f"RESOURCE_FS=addon-cpp-output total={usage.total} free={usage.free} "
          f"budget={EXTRA_BUDGET} reserve={reserve}")
    if usage.free < reserve + EXTRA_BUDGET:
        raise ContractError("RESOURCE_PREFLIGHT=FAIL")
    print("RESOURCE_PREFLIGHT=PASS")

    version = manifest["version"] + "~cpppreview1"
    target = output / f'{manifest["package"]}_{version}_{arch}.deb'
    if target.exists() or target.is_symlink():
        raise ContractError("refusing to overwrite compiled add-on package")

    with tempfile.TemporaryDirectory(prefix="suite-cpp-addon-", dir=output) as temporary:
        base = Path(temporary)
        package_root = base / "root"
        package_root.mkdir()
        stage(root, "rectools", str(package_root))
        libexec = package_root / "usr/libexec/vdr-suite/addons/rectools"
        libexec.mkdir(parents=True)
        payload = libexec / binary.name
        shutil.copyfile(binary, payload)
        payload.chmod(0o755)
        control = package_root / "DEBIAN"
        control.mkdir()
        (control / "control").write_text(
            f'Package: {manifest["package"]}\n'
            f'Version: {version}\n'
            f'Architecture: {arch}\n'
            'Section: utils\n'
            'Priority: optional\n'
            f'Maintainer: {maintainer}\n'
            'Depends: ffmpeg, vdr\n'
            'Description: Inactive VDR-Suite Media Tools C++ import preview\n'
            ' Standalone media remux import CLI, not connected to VDR-Suite.\n'
            ' No service, rights, jobs, API or activation is provided.\n',
            encoding="utf-8",
        )
        (control / "control").chmod(0o644)
        temporary_deb = base / "media-import.deb"
        try:
            subprocess.run(
                ["dpkg-deb", "--build", "--root-owner-group",
                 str(package_root), str(temporary_deb)],
                check=True, capture_output=True, text=True, timeout=60,
            )
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
            raise ContractError("dpkg-deb build failed") from exc
        if not temporary_deb.is_file() or temporary_deb.stat().st_size == 0:
            raise ContractError("missing compiled package payload")
        os.link(temporary_deb, target)

    if shutil.disk_usage(output).free < reserve:
        target.unlink(missing_ok=True)
        raise ContractError("RESOURCE_POSTCHECK=FAIL")
    print("RESOURCE_POSTCHECK=PASS")
    return target


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--maintainer", default="")
    args = parser.parse_args(argv)
    try:
        result = build(ROOT, args.binary, args.output_dir, args.maintainer)
        print("MEDIA_TOOLS_CPP_PACKAGE=PASS path=" + str(result))
    except (ContractError, OSError) as exc:
        print("MEDIA_TOOLS_CPP_PACKAGE=FAIL " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
