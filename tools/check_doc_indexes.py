#!/usr/bin/env python3
from pathlib import Path
from urllib.parse import unquote
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"

LINK_RE = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")
STRICT_ARCHIVE_NAMES = {"history", "completed-phases"}


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


def is_strict_archive(directory: Path) -> bool:
    rel_parts = directory.relative_to(DOCS).parts
    return any(part in STRICT_ARCHIVE_NAMES for part in rel_parts)


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

        indexes = [
            directory / name
            for name in ("index.md", "README.md")
            if (directory / name).exists()
        ]
        if not indexes:
            print(f"\nDirectory without index/README: {directory.relative_to(ROOT)}")
            for path in sorted(md_files):
                print(f"  - {path.name}")
            problems += len(md_files)
            continue

        linked = set()
        for index in indexes:
            linked |= linked_md_files(index)

        # Active/current indexes intentionally prioritize canonical entrypoints instead
        # of presenting every historical leaf at the same level. Archive directories,
        # however, must remain complete inventories for traceability.
        if is_strict_archive(directory):
            missing = sorted(md_files - linked)
            if missing:
                print(f"\nArchive index incomplete: {directory.relative_to(ROOT)}")
                for index in indexes:
                    print(f"  index: {index.name}")
                for path in missing:
                    print(f"  missing: {path.name}")
                problems += len(missing)
        elif not linked:
            print(f"\nActive directory index has no local Markdown links: {directory.relative_to(ROOT)}")
            problems += 1

    if problems:
        print(f"\nTotal index problems: {problems}")
        return 1

    print("Directory indexes and strict archives are valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
