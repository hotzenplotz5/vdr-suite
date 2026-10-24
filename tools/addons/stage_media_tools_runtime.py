#!/usr/bin/env python3
"""Compose a separately opted-in Media Tools Suite runtime stage, without dpkg or compilation.

Only an already sealed Suite runtime and an independently approved, already
compiled handler are accepted. This command does not deploy or restart services.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import stat
import sys
import tempfile

TOOLS = Path(__file__).resolve().parents[1]
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))
from install_runtime_stage import (  # noqa: E402
    MANIFEST_NAME, load_manifest, verify_sealed_stage, seal,
)
from addon_contract import validate_directory  # noqa: E402

WORKERS = ("vdr-suite-media-import", "vdr-suite-media-shrink",
           "vdr-suite-media-check")
METADATA = ("addon.json", "README.md", "AGENTS.md")


def regular(path: Path) -> bool:
    try:
        return stat.S_ISREG(path.lstat().st_mode)
    except OSError:
        return False


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def compose(work: Path, output: Path, expected_sha: str) -> None:
    if os.geteuid() == 0:
        raise ValueError("compile/stage as the unprivileged checkout owner, never root")
    if (len(expected_sha) != 64 or any(c not in "0123456789abcdef"
                                      for c in expected_sha)):
        raise ValueError("invalid independently approved handler SHA256")
    if (not work.is_absolute() or work.is_symlink() or not work.is_dir() or
            not output.is_absolute() or output.is_symlink() or
            output.exists() or not output.parent.is_dir() or
            output.parent.is_symlink()):
        raise ValueError("unsafe prepared workdir or output stage")
    candidate = work / "candidate.txt"
    if not regular(candidate):
        raise ValueError("missing prepared candidate record")
    lines = candidate.read_text(encoding="ascii").splitlines()
    if (len(lines) != 2 or len(lines[0]) != 40 or
            any(c not in "0123456789abcdef" for c in lines[0]) or
            lines[1] != expected_sha):
        raise ValueError("prepared handler approval differs")
    input_stage = work / "stage"
    if input_stage.is_symlink():
        raise ValueError("symlinked prepared runtime stage")
    verify_sealed_stage(input_stage, load_manifest(input_stage))
    source_metadata = REPO / "modules" / "rectools"
    manifest = validate_directory(source_metadata, "rectools")
    if manifest["version"] != "0.1.0":
        raise ValueError("unexpected Media Tools metadata version")
    workers = []
    for worker in WORKERS:
        path = work / "build" / "addons" / worker
        if (not regular(path) or not os.access(path, os.X_OK) or
                path.stat().st_size < 4 or path.stat().st_size > 16*1024*1024):
            raise ValueError("missing or unsafe already-built worker: " + worker)
        with path.open("rb") as stream:
            if stream.read(4) != b"\\x7fELF":
                raise ValueError("worker is not ELF: " + worker)
        workers.append(path)
    if digest(workers[0]) != expected_sha:
        raise ValueError("approved import-handler bytes changed")

    # Bound the copy before writing; leave >= 4 GiB or 15% on the output FS.
    stage_bytes = sum(p.stat().st_size for p in input_stage.rglob("*") if p.is_file())
    extra = sum(p.stat().st_size for p in workers)
    extra += sum((source_metadata / name).stat().st_size for name in METADATA)
    budget = stage_bytes + extra + 64*1024*1024
    disk = shutil.disk_usage(output.parent)
    reserve = max(4*1024**3, (disk.total*15+99)//100)
    print(f"RESOURCE_FS={output.parent} FREE={disk.free} BUDGET={budget} RESERVE={reserve}")
    if disk.free < budget + reserve:
        raise ValueError("RESOURCE_PREFLIGHT=FAIL")
    print("RESOURCE_PREFLIGHT=PASS")

    with tempfile.TemporaryDirectory(prefix=".media-tools-stage-", dir=output.parent) as scratch:
        stage = Path(scratch) / "stage"
        shutil.copytree(input_stage, stage)
        metadata_dir = stage / "usr/share/vdr-suite/addons/rectools"
        worker_dir = stage / "usr/libexec/vdr-suite/addons/rectools"
        if metadata_dir.exists() or worker_dir.exists():
            raise ValueError("Suite stage already includes Media Tools; refusing overwrite")
        metadata_dir.mkdir(parents=True)
        worker_dir.mkdir(parents=True)
        # Only the ISOLATED stage has its public directory modes set.
        # Deployment must never chmod pre-existing live host directories.
        for rel in ("usr/share/vdr-suite", "usr/share/vdr-suite/addons",
                    "usr/share/vdr-suite/addons/rectools",
                    "usr/libexec", "usr/libexec/vdr-suite",
                    "usr/libexec/vdr-suite/addons",
                    "usr/libexec/vdr-suite/addons/rectools"):
            os.chmod(stage / rel, 0o755)
        for name in METADATA:
            path = metadata_dir / name
            shutil.copyfile(source_metadata / name, path)
            os.chmod(path, 0o644)
        for binary in workers:
            path = worker_dir / binary.name
            shutil.copyfile(binary, path)
            os.chmod(path, 0o755)
        # Re-seal the complete, immutable Suite + optional add-on output.
        (stage / MANIFEST_NAME).unlink()
        seal(stage)
        verify_sealed_stage(stage, load_manifest(stage))
        os.replace(stage, output)

    after = shutil.disk_usage(output.parent)
    if after.free < reserve:
        raise ValueError("RESOURCE_POSTCHECK=FAIL (stage retained for inspection)")
    print("RESOURCE_POSTCHECK=PASS")
    print("MEDIA_TOOLS_RUNTIME_STAGE=PASS")
    print(f"PREPARED_SOURCE_SHA={lines[0]}")
    print(f"APPROVED_HANDLER_SHA256={expected_sha}")
    print(f"RUNTIME_STAGE={output}")
    print("BUILD=REUSED_NO_COMPILATION")
    print("DPKG=NOT_USED")
    print("LIVE_SYSTEM=UNCHANGED")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepared-workdir", required=True, type=Path)
    parser.add_argument("--output-stage", required=True, type=Path)
    parser.add_argument("--approved-handler-sha256", required=True)
    args = parser.parse_args()
    try:
        compose(args.prepared_workdir, args.output_stage,
                args.approved_handler_sha256)
    except (OSError, ValueError, RuntimeError) as exc:
        print("MEDIA_TOOLS_RUNTIME_STAGE=STOP " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
