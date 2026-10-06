#!/usr/bin/env python3
"""Seal, deploy and verify a canonical VDR-Suite install-runtime staging tree."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import tempfile
import uuid

MANIFEST_NAME = ".vdr-suite-install-runtime-manifest.json"
SCHEMA_VERSION = 2
FRONTEND_ROOT = Path("usr/share/vdr-suite/web/frontend")
REQUIRED_STAGE_FILES = (
    Path("usr/sbin/vdr-suite-daemon"),
    FRONTEND_ROOT / "index.html",
)


class DeploymentError(RuntimeError):
    pass


def fail(message: str) -> None:
    raise DeploymentError(message)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def path_mode(path: Path) -> int:
    return stat.S_IMODE(path.stat().st_mode)


def safe_relative(raw: str) -> Path:
    rel = Path(raw)
    if rel.is_absolute() or not rel.parts or any(part in ("", ".", "..") for part in rel.parts):
        fail(f"unsafe manifest path: {raw!r}")
    return rel


def iter_stage_tree(
    stage_root: Path,
) -> tuple[dict[str, int], dict[str, dict[str, object]]]:
    directories: dict[str, int] = {}
    files: dict[str, dict[str, object]] = {}
    for path in sorted(stage_root.rglob("*")):
        if path.name == MANIFEST_NAME and path.parent == stage_root:
            continue
        if path.is_symlink():
            fail(f"symlinks are not supported in install-runtime staging: {path}")
        rel = path.relative_to(stage_root).as_posix()
        if path.is_dir():
            directories[rel] = path_mode(path)
            continue
        if path.is_file():
            files[rel] = {
                "sha256": sha256_file(path),
                "mode": path_mode(path),
                "size": path.stat().st_size,
            }
    return directories, files


def require_stage_shape(stage_root: Path) -> None:
    if not stage_root.is_dir():
        fail(f"stage root does not exist: {stage_root}")
    for rel in REQUIRED_STAGE_FILES:
        path = stage_root / rel
        if not path.is_file():
            fail(f"stage is not a complete install-runtime tree; missing {rel.as_posix()}")


def manifest_path(stage_root: Path) -> Path:
    return stage_root / MANIFEST_NAME


def seal(stage_root: Path) -> None:
    require_stage_shape(stage_root)
    directories, files = iter_stage_tree(stage_root)
    payload = {
        "schema": SCHEMA_VERSION,
        "producer": "make stage-install-runtime",
        "frontend_root": FRONTEND_ROOT.as_posix(),
        "directories": directories,
        "files": files,
    }
    target = manifest_path(stage_root)
    temp = target.with_name(target.name + ".tmp")
    temp.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temp, target)
    print(
        "INSTALL_RUNTIME_STAGE_SEALED=YES "
        f"DIRS={len(directories)} FILES={len(files)} STAGE={stage_root}"
    )


def load_manifest(stage_root: Path) -> dict[str, object]:
    path = manifest_path(stage_root)
    if not path.is_file():
        fail(f"stage is not sealed; missing {MANIFEST_NAME}")
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail(f"cannot read stage manifest: {exc}")
    if payload.get("schema") != SCHEMA_VERSION:
        fail(f"unsupported stage manifest schema: {payload.get('schema')!r}")
    if payload.get("frontend_root") != FRONTEND_ROOT.as_posix():
        fail("stage manifest frontend root does not match the repository contract")
    directories = payload.get("directories")
    files = payload.get("files")
    if not isinstance(directories, dict) or not directories:
        fail("stage manifest does not contain install-runtime directories")
    if not isinstance(files, dict) or not files:
        fail("stage manifest does not contain install-runtime files")
    return payload


def verify_sealed_stage(
    stage_root: Path,
    payload: dict[str, object],
) -> tuple[dict[str, int], dict[str, dict[str, object]]]:
    require_stage_shape(stage_root)
    recorded_dirs = payload["directories"]
    recorded_files = payload["files"]
    assert isinstance(recorded_dirs, dict)
    assert isinstance(recorded_files, dict)

    current_dirs, current_files = iter_stage_tree(stage_root)
    if set(current_dirs) != set(recorded_dirs):
        missing = sorted(set(recorded_dirs) - set(current_dirs))
        extra = sorted(set(current_dirs) - set(recorded_dirs))
        fail(f"sealed stage directory set changed; missing={missing} extra={extra}")
    if set(current_files) != set(recorded_files):
        missing = sorted(set(recorded_files) - set(current_files))
        extra = sorted(set(current_files) - set(recorded_files))
        fail(f"sealed stage file set changed; missing={missing} extra={extra}")

    for rel, expected_mode in recorded_dirs.items():
        safe_relative(rel)
        if not isinstance(expected_mode, int):
            fail(f"invalid directory manifest entry: {rel}")
        if current_dirs[rel] != expected_mode:
            fail(f"sealed stage directory mode changed: {rel}")

    for rel, expected_raw in recorded_files.items():
        safe_relative(rel)
        if not isinstance(expected_raw, dict):
            fail(f"invalid file manifest entry: {rel}")
        actual = current_files[rel]
        if actual["sha256"] != expected_raw.get("sha256"):
            fail(f"sealed stage content changed: {rel}")
        if actual["mode"] != expected_raw.get("mode"):
            fail(f"sealed stage mode changed: {rel}")

    return recorded_dirs, recorded_files  # type: ignore[return-value]


def is_preserved_config(rel: Path) -> bool:
    return rel.parts and rel.parts[0] == "etc"


def is_frontend(rel: Path) -> bool:
    try:
        rel.relative_to(FRONTEND_ROOT)
        return True
    except ValueError:
        return False


def remove_path(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.exists():
        shutil.rmtree(path)


def ensure_directory(destination: Path, mode: int, preserve_existing: bool) -> None:
    if destination.exists() or destination.is_symlink():
        if not destination.is_dir() or destination.is_symlink():
            fail(f"live path is not a directory: {destination}")
        if not preserve_existing:
            os.chmod(destination, mode)
        return
    destination.mkdir(parents=True, exist_ok=False)
    os.chmod(destination, mode)


def atomic_copy_file(source: Path, destination: Path, mode: int) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=f".{destination.name}.vdr-suite-", dir=destination.parent)
    os.close(fd)
    temp = Path(temp_name)
    try:
        shutil.copyfile(source, temp)
        os.chmod(temp, mode)
        os.replace(temp, destination)
    finally:
        if temp.exists():
            temp.unlink()


def replace_frontend_tree(stage_root: Path, live_root: Path) -> None:
    source = stage_root / FRONTEND_ROOT
    destination = live_root / FRONTEND_ROOT
    parent = destination.parent
    parent.mkdir(parents=True, exist_ok=True)

    candidate = Path(tempfile.mkdtemp(prefix=".vdr-suite-frontend-new-", dir=parent))
    backup = parent / f".vdr-suite-frontend-old-{uuid.uuid4().hex}"
    moved_old = False
    installed_new = False
    try:
        shutil.copytree(source, candidate, dirs_exist_ok=True, copy_function=shutil.copy2)
        os.chmod(candidate, path_mode(source))
        if destination.exists() or destination.is_symlink():
            os.replace(destination, backup)
            moved_old = True
        os.replace(candidate, destination)
        installed_new = True
        if moved_old:
            remove_path(backup)
    except Exception:
        if installed_new and (destination.exists() or destination.is_symlink()):
            remove_path(destination)
        if moved_old and (backup.exists() or backup.is_symlink()):
            os.replace(backup, destination)
        raise
    finally:
        if candidate.exists():
            remove_path(candidate)
        if backup.exists() or backup.is_symlink():
            remove_path(backup)


def deployment_differences(
    live_root: Path,
    recorded_dirs: dict[str, int],
    recorded_files: dict[str, dict[str, object]],
) -> list[str]:
    differences: list[str] = []
    stage_frontend_dirs: set[str] = set()
    stage_frontend_files: set[str] = set()
    live_frontend_dirs: set[str] = set()
    live_frontend_files: set[str] = set()

    for rel_raw, expected_mode in recorded_dirs.items():
        rel = safe_relative(rel_raw)
        live_dir = live_root / rel
        if is_frontend(rel):
            stage_frontend_dirs.add(rel.relative_to(FRONTEND_ROOT).as_posix())
        if is_preserved_config(rel) and live_dir.is_dir() and not live_dir.is_symlink():
            continue
        if not live_dir.is_dir() or live_dir.is_symlink():
            differences.append(f"missing-dir:{rel.as_posix()}")
            continue
        if path_mode(live_dir) != expected_mode:
            differences.append(f"dir-mode:{rel.as_posix()}")

    for rel_raw, expected in recorded_files.items():
        rel = safe_relative(rel_raw)
        live_file = live_root / rel

        if is_frontend(rel):
            stage_frontend_files.add(rel.relative_to(FRONTEND_ROOT).as_posix())

        if is_preserved_config(rel) and live_file.exists():
            continue
        if not live_file.is_file() or live_file.is_symlink():
            differences.append(f"missing:{rel.as_posix()}")
            continue
        if sha256_file(live_file) != expected["sha256"]:
            differences.append(f"content:{rel.as_posix()}")
            continue
        if path_mode(live_file) != expected["mode"]:
            differences.append(f"mode:{rel.as_posix()}")

    live_frontend = live_root / FRONTEND_ROOT
    if live_frontend.is_dir() and not live_frontend.is_symlink():
        live_frontend_dirs.add(".")
        for path in live_frontend.rglob("*"):
            if path.is_symlink():
                differences.append(f"frontend-symlink:{path.relative_to(live_frontend).as_posix()}")
            elif path.is_dir():
                live_frontend_dirs.add(path.relative_to(live_frontend).as_posix())
            elif path.is_file():
                live_frontend_files.add(path.relative_to(live_frontend).as_posix())
    elif stage_frontend_dirs or stage_frontend_files:
        differences.append(f"missing-tree:{FRONTEND_ROOT.as_posix()}")

    for missing in sorted(stage_frontend_dirs - live_frontend_dirs):
        marker = f"missing-dir:{(FRONTEND_ROOT / missing).as_posix()}"
        if marker not in differences:
            differences.append(marker)
    for extra in sorted(live_frontend_dirs - stage_frontend_dirs):
        differences.append(f"extra-dir:{(FRONTEND_ROOT / extra).as_posix()}")
    for missing in sorted(stage_frontend_files - live_frontend_files):
        marker = f"missing:{(FRONTEND_ROOT / missing).as_posix()}"
        if marker not in differences:
            differences.append(marker)
    for extra in sorted(live_frontend_files - stage_frontend_files):
        differences.append(f"extra:{(FRONTEND_ROOT / extra).as_posix()}")

    return differences


def check(stage_root: Path, live_root: Path) -> None:
    payload = load_manifest(stage_root)
    recorded_dirs, recorded_files = verify_sealed_stage(stage_root, payload)
    differences = deployment_differences(live_root, recorded_dirs, recorded_files)
    if differences:
        for item in differences:
            print(f"RUNTIME_DEPLOYMENT_DRIFT={item}")
        fail(f"runtime deployment does not match sealed install-runtime stage ({len(differences)} differences)")
    print(f"RUNTIME_DEPLOYMENT_MATCH=YES STAGE={stage_root} LIVE_ROOT={live_root}")


def deploy(stage_root: Path, live_root: Path) -> None:
    payload = load_manifest(stage_root)
    recorded_dirs, recorded_files = verify_sealed_stage(stage_root, payload)

    for rel_raw, expected_mode in sorted(
        recorded_dirs.items(),
        key=lambda item: (len(Path(item[0]).parts), item[0]),
    ):
        rel = safe_relative(rel_raw)
        if is_frontend(rel):
            continue
        ensure_directory(
            live_root / rel,
            expected_mode,
            preserve_existing=is_preserved_config(rel),
        )

    for rel_raw, expected in recorded_files.items():
        rel = safe_relative(rel_raw)
        if is_frontend(rel):
            continue
        source = stage_root / rel
        destination = live_root / rel
        if is_preserved_config(rel) and destination.exists():
            print(f"PRESERVE_EXISTING_CONFIG={destination}")
            continue
        atomic_copy_file(source, destination, int(expected["mode"]))

    replace_frontend_tree(stage_root, live_root)
    check(stage_root, live_root)
    print(f"INSTALL_RUNTIME_DEPLOYED=YES STAGE={stage_root} LIVE_ROOT={live_root}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("seal", "check", "deploy"):
        command = sub.add_parser(name)
        command.add_argument("--stage-root", type=Path, required=True)
        if name != "seal":
            command.add_argument("--live-root", type=Path, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        stage_root = args.stage_root.resolve()
        if args.command == "seal":
            seal(stage_root)
        elif args.command == "check":
            check(stage_root, args.live_root.resolve())
        elif args.command == "deploy":
            deploy(stage_root, args.live_root.resolve())
        else:
            fail(f"unsupported command: {args.command}")
    except DeploymentError as exc:
        print(f"ERROR: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
