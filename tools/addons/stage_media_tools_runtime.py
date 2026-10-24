#!/usr/bin/env python3
"""Compose a separately opted-in Media Tools Suite runtime stage, without dpkg or compilation.

Only an already sealed Suite runtime and an independently approved, already
compiled handler are accepted. This command does not deploy or restart services.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
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
INVENTORY_REL = Path("usr/share/vdr-suite/addons/rectools/media-tools-install-inventory.json")
SHARED_REL = Path("usr/share/vdr-suite/addons/rectools")
WORKER_REL = Path("usr/libexec/vdr-suite/addons/rectools")


def payload_roles() -> dict[str, str]:
    """Exactly the add-on-owned payload inside the canonical Suite runtime stage."""
    result = {(SHARED_REL / name).as_posix(): "metadata" for name in METADATA}
    result[(WORKER_REL / "addon.json").as_posix()] = "agent-metadata"
    result.update({(WORKER_REL / name).as_posix(): "worker" for name in WORKERS})
    return result


def verify_media_tools_stage(stage: Path, approved_sha256: str) -> dict:
    """Check the sealed stage and its versioned add-on payload; no host writes.

    The stage seal is not a signature and never grants handler execution.
    Callers must obtain approved_sha256 independently of the stage itself.
    """
    if not re.fullmatch(r"[0-9a-f]{64}", approved_sha256):
        raise ValueError("invalid independently approved handler SHA256")
    if not stage.is_absolute() or stage.is_symlink():
        raise ValueError("unsafe Media Tools stage root")
    _, sealed_files = verify_sealed_stage(stage, load_manifest(stage))
    inventory_path = stage / INVENTORY_REL
    if not regular(inventory_path) or inventory_path.stat().st_size > 16384:
        raise ValueError("missing or invalid Media Tools install inventory")
    if INVENTORY_REL.as_posix() not in sealed_files:
        raise ValueError("Media Tools inventory is not in the stage seal")
    try:
        record = json.loads(inventory_path.read_text(encoding="utf-8"))
        module = json.loads((stage / SHARED_REL / "addon.json").read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ValueError("invalid Media Tools metadata") from exc
    if (not isinstance(record, dict) or set(record) != {
            "schema", "module", "package", "version", "prepared_source_sha",
            "approved_handler_sha256", "files"} or record["schema"] != 1 or
            record["module"] != "rectools" or
            not isinstance(record["prepared_source_sha"], str) or
            not re.fullmatch(r"[0-9a-f]{40}", record["prepared_source_sha"]) or
            record["approved_handler_sha256"] != approved_sha256):
        raise ValueError("Media Tools inventory identity/schema mismatch")
    if (module.get("state") != "scaffold" or
            record["version"] != module.get("version") or
            record["package"] != module.get("package")):
        raise ValueError("Media Tools staged metadata and version differ")
    expected = payload_roles()
    entries = record["files"]
    if not isinstance(entries, dict) or set(entries) != set(expected):
        raise ValueError("Media Tools payload inventory is incomplete or unexpected")
    for rel, role in expected.items():
        entry = entries[rel]
        if (rel not in sealed_files or not isinstance(entry, dict) or
                set(entry) != {"role", "sha256", "size", "mode"} or
                entry["role"] != role or
                any(entry[key] != sealed_files[rel][key]
                    for key in ("sha256", "size", "mode"))):
            raise ValueError("Media Tools payload differs: " + rel)
    if entries[(WORKER_REL / WORKERS[0]).as_posix()]["sha256"] != approved_sha256:
        raise ValueError("Media Tools handler differs from independently approved bytes")
    if (stage / WORKER_REL / "addon.json").read_bytes() != (
            stage / SHARED_REL / "addon.json").read_bytes():
        raise ValueError("Media Tools metadata companion mismatch")
    return record



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
            if stream.read(4) != b"\x7fELF":
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
        # Keep the descriptive FHS inventory, but let the unprivileged Agent
        # verify a root-owned metadata companion beside the pinned handler.
        # Existing live /usr/share/vdr-suite modes are never changed.
        agent_manifest = worker_dir / "addon.json"
        shutil.copyfile(source_metadata / "addon.json", agent_manifest)
        os.chmod(agent_manifest, 0o644)
        # One exact, versioned list for manual deployment and future .deb.
        # Neither private Suite state nor config files belong in this payload.
        inventory = {
            "schema": 1,
            "module": "rectools",
            "package": manifest["package"],
            "version": manifest["version"],
            "prepared_source_sha": lines[0],
            "approved_handler_sha256": expected_sha,
            "files": {
                rel: {
                    "role": role,
                    "sha256": digest(stage / rel),
                    "size": (stage / rel).stat().st_size,
                    "mode": stat.S_IMODE((stage / rel).stat().st_mode),
                }
                for rel, role in sorted(payload_roles().items())
            },
        }
        inventory_path = stage / INVENTORY_REL
        inventory_path.write_text(
            json.dumps(inventory, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        os.chmod(inventory_path, 0o644)
        # Re-seal the complete, immutable Suite + optional add-on output.
        (stage / MANIFEST_NAME).unlink()
        seal(stage)
        verify_media_tools_stage(stage, expected_sha)
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
    parser.add_argument("--prepared-workdir", type=Path)
    parser.add_argument("--output-stage", type=Path)
    parser.add_argument("--verify-stage", type=Path,
                        help="read-only stage and add-on inventory verification")
    parser.add_argument("--approved-handler-sha256", required=True)
    args = parser.parse_args()
    try:
        if args.verify_stage is not None:
            if args.prepared_workdir is not None or args.output_stage is not None:
                parser.error("--verify-stage cannot compose a stage")
            record = verify_media_tools_stage(args.verify_stage,
                                              args.approved_handler_sha256)
            print("MEDIA_TOOLS_STAGE_INVENTORY=PASS")
            print("PREPARED_SOURCE_SHA=" + record["prepared_source_sha"])
            print("MEDIA_TOOLS_PACKAGE=" + record["package"])
            print("MEDIA_TOOLS_VERSION=" + record["version"])
        else:
            if args.prepared_workdir is None or args.output_stage is None:
                parser.error("composition needs prepared workdir and output stage")
            compose(args.prepared_workdir, args.output_stage,
                    args.approved_handler_sha256)
    except (OSError, ValueError, RuntimeError) as exc:
        print("MEDIA_TOOLS_RUNTIME_STAGE=STOP " + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
