#!/usr/bin/env python3
"""Synchronize channel logos into the VDR-Suite logo cache.

The browser and TV frontends should not hotlink remote images. This helper keeps
logos local by downloading an archive or copying an existing logo tree into a
cache directory that the daemon can serve through /channel-logos/.
"""

from __future__ import annotations

import argparse
import os
import shutil
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

DEFAULT_SOURCE_URL = "https://github.com/3PO/Senderlogos/archive/refs/heads/master.zip"
DEFAULT_TARGET_DIR = "/var/cache/vdr-suite/channel-logos"
SUPPORTED_SUFFIXES = {".png", ".svg"}


def normalize_logo_name(name: str) -> str:
    return " ".join(name.strip().lower().split())


def is_supported_logo(path: Path) -> bool:
    return path.suffix.lower() in SUPPORTED_SUFFIXES


def safe_output_name(path: Path) -> str:
    return normalize_logo_name(path.name)


def copy_logo(source: Path, target_dir: Path, dry_run: bool) -> bool:
    if not is_supported_logo(source):
        return False

    target_name = safe_output_name(source)
    if not target_name:
        return False

    target = target_dir / target_name

    if dry_run:
        print(f"would copy {source} -> {target}")
        return True

    target_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    return True


def sync_from_directory(source_dir: Path, target_dir: Path, dry_run: bool) -> int:
    count = 0

    for path in sorted(source_dir.rglob("*")):
        if path.is_file() and copy_logo(path, target_dir, dry_run):
            count += 1

    return count


def download_archive(source_url: str, target_path: Path) -> None:
    request = urllib.request.Request(
        source_url,
        headers={"User-Agent": "vdr-suite-logo-sync"},
    )

    with urllib.request.urlopen(request, timeout=60) as response:
        target_path.write_bytes(response.read())


def sync_from_zip(source_url: str, target_dir: Path, dry_run: bool) -> int:
    with tempfile.TemporaryDirectory(prefix="vdr-suite-logo-sync-") as tmp:
        tmp_dir = Path(tmp)
        archive = tmp_dir / "logos.zip"
        extract_dir = tmp_dir / "extract"

        print(f"downloading {source_url}")
        download_archive(source_url, archive)

        with zipfile.ZipFile(archive) as zip_file:
            zip_file.extractall(extract_dir)

        return sync_from_directory(extract_dir, target_dir, dry_run)


def run_self_test() -> int:
    with tempfile.TemporaryDirectory(prefix="vdr-suite-logo-sync-test-") as tmp:
        root = Path(tmp)
        source = root / "source"
        target = root / "target"
        source.mkdir()
        (source / "ZDF Neo HD.PNG").write_bytes(b"png")
        (source / "ignore.txt").write_text("ignored", encoding="utf-8")

        copied = sync_from_directory(source, target, dry_run=False)
        expected = target / "zdf neo hd.png"

        if copied != 1 or not expected.exists():
            print("self-test failed", file=sys.stderr)
            return 1

    print("self-test ok")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Synchronize channel logos into the VDR-Suite logo cache.",
    )
    parser.add_argument(
        "--source-url",
        default=os.environ.get("VDR_SUITE_CHANNEL_LOGO_SYNC_URL", DEFAULT_SOURCE_URL),
        help="ZIP archive URL to download when --source-dir is not set.",
    )
    parser.add_argument(
        "--source-dir",
        help="Copy logos from a local directory instead of downloading an archive.",
    )
    parser.add_argument(
        "--target-dir",
        default=os.environ.get("VDR_SUITE_CHANNEL_LOGO_CACHE", DEFAULT_TARGET_DIR),
        help="Target cache directory.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print intended copies without writing files.",
    )
    parser.add_argument(
        "--self-test",
        action="store_true",
        help="Run an offline self-test and exit.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.self_test:
        return run_self_test()

    target_dir = Path(args.target_dir)

    if args.source_dir:
        source_dir = Path(args.source_dir)
        if not source_dir.is_dir():
            print(f"source directory not found: {source_dir}", file=sys.stderr)
            return 2
        count = sync_from_directory(source_dir, target_dir, args.dry_run)
    else:
        count = sync_from_zip(args.source_url, target_dir, args.dry_run)

    action = "would sync" if args.dry_run else "synced"
    print(f"{action} {count} logo file(s) into {target_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
