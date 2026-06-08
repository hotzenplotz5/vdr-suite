#!/usr/bin/env python3
from pathlib import Path
from urllib.parse import unquote
import re
import os
import sys

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

LINK_RE = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")

def strip_target(raw: str) -> str:
    target = raw.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1].strip()
    if " " in target:
        first, rest = target.split(" ", 1)
        if rest.strip().startswith('"') or rest.strip().startswith("'"):
            target = first
    return unquote(target).split("#", 1)[0]

def linked_md_files(index: Path) -> set[Path]:
    if not index.exists():
        return set()

    text = index.read_text(encoding="utf-8")
    result = set()

    for match in LINK_RE.finditer(text):
        raw = strip_target(match.group(1))
        if not raw or raw.startswith("http://") or raw.startswith("https://"):
            continue
        candidate = (index.parent / raw).resolve()
        try:
            candidate.relative_to(ROOT)
        except ValueError:
            continue
        if candidate.exists() and candidate.suffix == ".md":
            result.add(candidate)

    return result

def main() -> int:
    problems = 0

    for directory in sorted([p for p in DOCS.rglob("*") if p.is_dir()] + [DOCS]):
        md_files = {
            p.resolve()
            for p in directory.glob("*.md")
            if p.name not in {"index.md", "README.md"}
        }

        if not md_files:
            continue

        indexes = []
        for name in ("index.md", "README.md"):
            idx = directory / name
            if idx.exists():
                indexes.append(idx)

        if not indexes:
            print(f"\nDirectory without index/README: {directory.relative_to(ROOT)}")
            for p in sorted(md_files):
                print(f"  - {p.name}")
            problems += len(md_files)
            continue

        linked = set()
        for idx in indexes:
            linked |= linked_md_files(idx)

        missing = sorted(md_files - linked)

        if missing:
            print(f"\nDirectory index incomplete: {directory.relative_to(ROOT)}")
            for idx in indexes:
                print(f"  index: {idx.name}")
            for p in missing:
                print(f"  missing: {p.name}")
            problems += len(missing)

    if problems:
        print(f"\nTotal missing local index links: {problems}")
        return 1

    print("All local directory indexes are complete.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
