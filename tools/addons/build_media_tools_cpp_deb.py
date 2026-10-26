#!/usr/bin/env python3
"""Package the separately compiled Media Tools C++ preview without installing it.

Unlike the metadata-only ~scaffold1 packages, this owns one offline CLI under
private libexec; the Suite runtime never discovers or activates the binary.
"""
from __future__ import annotations

import argparse
import os
import shlex
import stat
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from addon_contract import ROOT, ContractError, stage, validate_all, validate_stage_root
from build_deb import validate_maintainer
from stage_media_tools_runtime import (
    INVENTORY_REL, payload_roles, verify_media_tools_stage,
)

EXTRA_BUDGET = 64 * 1024 * 1024


def build(root: Path, executable: str, output_dir: str, maintainer: str,
          shrink_binary: str | None = None,
          check_binary: str | None = None) -> Path:
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
    extras = []
    for expected, requested in (
        ("vdr-suite-media-shrink", shrink_binary),
        ("vdr-suite-media-check", check_binary),
    ):
        if requested is None:
            continue
        candidate = Path(requested)
        if (not candidate.is_absolute() or candidate.name != expected
                or candidate.is_symlink() or not candidate.is_file()
                or not os.access(candidate, os.X_OK)
                or candidate.stat().st_size < 4
                or candidate.stat().st_size > 16 * 1024 * 1024):
            raise ContractError(f"invalid extra media tools binary: {expected}")
        with candidate.open("rb") as handle:
            if handle.read(4) != b"\x7fELF":
                raise ContractError(f"extra media tools binary is not ELF: {expected}")
        extras.append(candidate)
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
        for source in [binary, *extras]:
            payload = libexec / source.name
            shutil.copyfile(source, payload)
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
            'Description: Inactive VDR-Suite Media Tools C++ workers\n'
            ' Source-built import, read-only check and HEVC review CLI payloads.\n'
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




def stage_package_paths() -> tuple[str, ...]:
    """Exact package-owned files from the sealed manual-deployment stage."""
    return tuple(sorted((*payload_roles(), INVENTORY_REL.as_posix())))


def collision_preinst(paths: tuple[str, ...], *, test_root: Path = Path("/")) -> str:
    """No takeover of manually deployed or foreign-owned runtime files.

    No service starts, grants, identity changes, cleanup, or postinst hooks.
    A test-only root prefix exercises the *same* shell guard without ever
    probing paths on the operator's host.
    """
    if not test_root.is_absolute() or test_root.is_symlink():
        raise ContractError("invalid preinst test root")
    parents = set()
    targets = []
    for rel in paths:
        if not rel.startswith("usr/") or ".." in Path(rel).parts:
            raise ContractError("unexpected add-on package path")
        target = test_root / rel
        targets.append(str(target))
        parts = Path(rel).parts
        for depth in range(1, len(parts)):
            parents.add(str(test_root.joinpath(*parts[:depth])))
    return (
        "#!/bin/sh\n"
        "set -eu\n"
        "package='vdr-suite-addon-media-tools'\n"
        "deny() { printf 'MEDIA_TOOLS_DPKG=BLOCKED %s\\n' \"$1\" >&2; exit 1; }\n"
        "for parent in " + " ".join(shlex.quote(p) for p in sorted(parents)) + "; do\n"
        "  [ ! -L \"$parent\" ] || deny 'symlinked package directory'\n"
        "  [ ! -e \"$parent\" ] || [ -d \"$parent\" ] || deny 'non-directory ancestor'\n"
        "done\n"
        "for target in " + " ".join(shlex.quote(p) for p in targets) + "; do\n"
        "  if [ -e \"$target\" ] || [ -L \"$target\" ]; then\n"
        "    [ -f \"$target\" ] && [ ! -L \"$target\" ] || deny 'nonregular existing path'\n"
        "    owner=$(dpkg-query -S \"$target\" 2>/dev/null) || deny 'manually installed path'\n"
        "    [ \"$owner\" = \"$package: $target\" ] || deny 'foreign package ownership'\n"
        "  fi\n"
        "done\n"
        "exit 0\n"
    )


def build_from_sealed_stage(stage: Path, approved: str, output_dir: str,
                            maintainer: str) -> Path:
    """MT.I4: an opt-in, *inactive* add-on .deb using the MT.I1 stage bytes.

    Neither this offline package nor installation of it grants the add-on any
    runtime capability; installed code still needs the separate Core approval.
    This is not a live install or a migration of manual/deb-owned Suite files.
    """
    maintainer = validate_maintainer(maintainer)
    output, _ = validate_stage_root(output_dir, "/usr")
    record = verify_media_tools_stage(stage, approved)
    paths = stage_package_paths()
    if set(paths) != set(record["files"]) | {INVENTORY_REL.as_posix()}:
        raise ContractError("sealed stage package file list differs")
    if not shutil.which("dpkg-deb") or not shutil.which("dpkg"):
        raise ContractError("dpkg tools unavailable")
    try:
        arch = subprocess.check_output(
            ["dpkg", "--print-architecture"], text=True, timeout=5,
        ).strip()
    except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        raise ContractError("cannot determine package architecture") from exc
    if not arch or not arch.isascii() or not arch.replace("-", "").isalnum():
        raise ContractError("invalid package architecture")
    # A same-source build is deterministic and cannot clobber another package.
    version = record["version"] + "~stage1+git" + record["prepared_source_sha"][:12]
    name = record["package"] + "_" + version + "_" + arch + ".deb"
    target = output / name
    if target.exists() or target.is_symlink():
        raise ContractError("refusing to overwrite sealed-stage package")
    total = sum((stage / rel).stat().st_size for rel in paths)
    usage = shutil.disk_usage(output)
    reserve = max(4 * 1024**3, (usage.total * 15 + 99) // 100)
    budget = total * 3 + EXTRA_BUDGET
    print(f"RESOURCE_FS=addon-stage-output total={usage.total} free={usage.free} "
          f"budget={budget} reserve={reserve}")
    if usage.free < budget + reserve:
        raise ContractError("RESOURCE_PREFLIGHT=FAIL")
    print("RESOURCE_PREFLIGHT=PASS")
    with tempfile.TemporaryDirectory(prefix="suite-sealed-addon-", dir=output) as scratch:
        workspace = Path(scratch)
        root = workspace / "package-root"
        root.mkdir()
        for rel in paths:
            origin = stage / rel
            if origin.is_symlink() or not origin.is_file():
                raise ContractError("invalid sealed stage file: " + rel)
            destination = root / rel
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin, destination)
            destination.chmod(stat.S_IMODE(origin.stat().st_mode))
        control = root / "DEBIAN"
        control.mkdir()
        (control / "control").write_text(
            f"Package: {record['package']}\n"
            f"Version: {version}\n"
            f"Architecture: {arch}\n"
            "Section: utils\n"
            "Priority: optional\n"
            f"Maintainer: {maintainer}\n"
            "Depends: ffmpeg, vdr\n"
            "Description: Inactive VDR-Suite Media Tools stage payload\n"
            " Exact sealed manual-stage worker and metadata bytes.\n"
            " Installation does not activate operations, grants or services.\n",
            encoding="utf-8",
        )
        (control / "control").chmod(0o644)
        preinst = control / "preinst"
        preinst.write_text(collision_preinst(paths), encoding="utf-8")
        preinst.chmod(0o755)
        draft = workspace / "candidate.deb"
        try:
            subprocess.run(
                ["dpkg-deb", "--build", "--root-owner-group", str(root), str(draft)],
                check=True, capture_output=True, text=True, timeout=60,
            )
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
            raise ContractError("sealed-stage dpkg-deb build failed") from exc
        extracted = workspace / "extracted"
        subprocess.run(["dpkg-deb", "--extract", str(draft), str(extracted)],
                       check=True, capture_output=True, text=True, timeout=30)
        from stage_media_tools_runtime import digest
        for rel in paths:
            source = stage / rel
            result = extracted / rel
            if (not result.is_file() or result.is_symlink() or
                    digest(source) != digest(result) or
                    stat.S_IMODE(source.stat().st_mode) !=
                    stat.S_IMODE(result.stat().st_mode)):
                raise ContractError("deb payload differs from sealed stage: " + rel)
        packaged = {
            str(p.relative_to(extracted)) for p in extracted.rglob("*") if p.is_file()
        }
        if packaged != set(paths):
            raise ContractError("unexpected package files outside the add-on inventory")
        os.link(draft, target)
    if shutil.disk_usage(output).free < reserve:
        target.unlink(missing_ok=True)
        raise ContractError("RESOURCE_POSTCHECK=FAIL")
    print("RESOURCE_POSTCHECK=PASS")
    print("MEDIA_TOOLS_STAGE_DEB=PASS")
    print("STAGED_SOURCE_SHA=" + record["prepared_source_sha"])
    print("PACKAGE_BUILD=REUSED_STAGE_NO_COMPILATION")
    print("LIVE_SYSTEM=UNCHANGED")
    return target

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary")
    parser.add_argument("--shrink-binary")
    parser.add_argument("--check-binary")
    parser.add_argument("--sealed-stage", type=Path)
    parser.add_argument("--approved-handler-sha256")
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--maintainer", default="")
    args = parser.parse_args(argv)
    try:
        if args.sealed_stage is not None:
            if args.binary or args.shrink_binary or args.check_binary or (
                    not args.approved_handler_sha256):
                parser.error("sealed-stage mode takes only stage, approved SHA, output and maintainer")
            result = build_from_sealed_stage(
                args.sealed_stage, args.approved_handler_sha256,
                args.output_dir, args.maintainer,
            )
            print("MEDIA_TOOLS_STAGE_PACKAGE=PASS path=" + str(result))
        else:
            if not args.binary or args.approved_handler_sha256:
                parser.error("preview mode needs --binary, without stage approval")
            result = build(ROOT, args.binary, args.output_dir, args.maintainer,
                           args.shrink_binary, args.check_binary)
            print("MEDIA_TOOLS_CPP_PACKAGE=PASS path=" + str(result))
    except (ContractError, OSError, ValueError, subprocess.CalledProcessError,
            subprocess.TimeoutExpired) as exc:
        print("MEDIA_TOOLS_PACKAGE=FAIL " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
