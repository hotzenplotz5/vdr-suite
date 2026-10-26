#!/usr/bin/env python3
"""Prepare Media Tools from an existing sealed Suite stage and already built workers.

No compilation, live deployment, service action, root privileges or approval.
Output is created atomically under an unprivileged, new workdir.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile

TOOLS = Path(__file__).resolve().parents[1]
REPO = TOOLS.parent
sys.path.insert(0, str(TOOLS))
from install_runtime_stage import load_manifest, verify_sealed_stage
from stage_media_tools_runtime import WORKERS


def regular_executable(path: Path) -> bool:
    try:
        info = path.lstat()
        return (stat.S_ISREG(info.st_mode) and info.st_nlink == 1 and
                info.st_size >= 4 and info.st_size <= 16 * 1024 * 1024 and
                bool(info.st_mode & stat.S_IXUSR) and
                not bool(info.st_mode & 0o022) and
                path.open("rb").read(4) == b"\x7fELF")
    except OSError:
        return False


def prepare(base: Path, workers: Path, output: Path,
            worker_source_sha: str) -> str:
    if os.geteuid() == 0:
        raise ValueError("prepare as an unprivileged user")
    for label, path in (("Suite stage", base), ("worker directory", workers)):
        if not path.is_absolute() or path.is_symlink() or not path.is_dir():
            raise ValueError("unsafe " + label)
    if (not output.is_absolute() or output.is_symlink() or output.exists() or
            not output.parent.is_dir() or output.parent.is_symlink()):
        raise ValueError("new absolute output workdir required")
    verify_sealed_stage(base, load_manifest(base))
    if any(not regular_executable(workers / name) for name in WORKERS):
        raise ValueError("all three prebuilt, safe Media Tools workers required")
    result = subprocess.run(["git", "-C", str(REPO), "rev-parse", "HEAD"],
                            check=True, text=True, capture_output=True, timeout=10)
    checkout = result.stdout.strip()
    if not re.fullmatch(r"[0-9a-f]{40}", checkout):
        raise ValueError("invalid checkout commit")
    if not re.fullmatch(r"[0-9a-f]{40}", worker_source_sha):
        raise ValueError("exact previous worker source commit required")
    # Reusing already built worker bytes is valid only when their C++ source,
    # public interfaces and add-on metadata are byte-identical to checkout.
    # The worker build may come from the GitHub merge commit while this
    # maintenance checkout is on its PR branch. Both refs need to exist,
    # but ancestry is not required; compare the actual worker sources.
    subprocess.run(["git", "-C", str(REPO), "cat-file", "-e",
                    worker_source_sha + "^{commit}"], check=True, timeout=10)
    subprocess.run(["git", "-C", str(REPO), "diff", "--quiet",
                    worker_source_sha, checkout, "--",
                    "addons/media-tools/src", "addons/media-tools/include",
                    "modules/rectools/addon.json"],
                   check=True, timeout=10)
    commit = worker_source_sha
    needed = sum(p.stat().st_size for p in base.rglob("*") if p.is_file())
    needed += sum((workers / name).stat().st_size for name in WORKERS)
    disk = shutil.disk_usage(output.parent)
    reserve = max(4 * 1024 ** 3, (disk.total * 15 + 99) // 100)
    if disk.free < needed + reserve:
        raise ValueError("RESOURCE_PREFLIGHT=FAIL")

    with tempfile.TemporaryDirectory(prefix=".media-tools-prepare-", dir=output.parent) as scratch:
        root = Path(scratch) / "work"
        root.mkdir(mode=0o700)
        shutil.copytree(base, root / "stage")
        verify_sealed_stage(root / "stage", load_manifest(root / "stage"))
        target = root / "build/addons"
        target.mkdir(parents=True)
        for name in WORKERS:
            destination = target / name
            shutil.copyfile(workers / name, destination)
            destination.chmod(0o755)
            if not regular_executable(destination) or hash_file(destination) != hash_file(workers / name):
                raise ValueError("worker copy mismatch: " + name)
        (root / "source-commit.txt").write_text(commit + "\n", encoding="ascii")
        (root / "source-commit.txt").chmod(0o600)
        os.replace(root, output)
    return hash_file(output / "build/addons" / WORKERS[0])


def hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-stage", required=True, type=Path)
    parser.add_argument("--worker-directory", required=True, type=Path)
    parser.add_argument("--output-workdir", required=True, type=Path)
    parser.add_argument("--worker-source-commit", required=True)
    args = parser.parse_args()
    try:
        sha = prepare(args.base_stage, args.worker_directory,
                      args.output_workdir, args.worker_source_commit)
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as exc:
        print("MEDIA_TOOLS_PREPARE=STOP " + str(exc), file=sys.stderr)
        return 1
    print("MEDIA_TOOLS_PREPARE=PASS")
    print("PREBUILT_WORKERS=REUSED")
    print("SUITE_STAGE=REUSED_NO_DAEMON_BUILD")
    print("NEW_HANDLER_SHA256=" + sha)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
