#!/usr/bin/env python3
from pathlib import Path
from urllib.parse import unquote
import re
import sys
import os
from collections import deque

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
DOCS = ROOT / "docs"

LINK_RE = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")

SKIP_PREFIXES = (
    "http://",
    "https://",
    "mailto:",
    "tel:",
)

def strip_link_target(raw: str) -> str:
    target = raw.strip()

    if not target:
        return target

    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1].strip()

    if " " in target:
        first, rest = target.split(" ", 1)
        if rest.strip().startswith('"') or rest.strip().startswith("'"):
            target = first

    return unquote(target)

def is_external(target: str) -> bool:
    lower = target.lower()
    return lower.startswith(SKIP_PREFIXES)

def resolve_markdown_link(source: Path, raw_target: str) -> Path | None:
    target = strip_link_target(raw_target)

    if not target:
        return None

    if is_external(target):
        return None

    if target.startswith("#"):
        return None

    target_without_anchor = target.split("#", 1)[0]

    if not target_without_anchor:
        return None

    if target_without_anchor.startswith("/"):
        candidate = ROOT / target_without_anchor.lstrip("/")
    else:
        candidate = source.parent / target_without_anchor

    candidate = candidate.resolve()

    try:
        candidate.relative_to(ROOT)
    except ValueError:
        return None

    if candidate.is_dir():
        index = candidate / "README.md"
        if index.exists():
            candidate = index
        else:
            index = candidate / "index.md"
            if index.exists():
                candidate = index

    if candidate.suffix.lower() != ".md":
        return None

    if not candidate.exists():
        return None

    return candidate

def outgoing_markdown_links(path: Path) -> list[Path]:
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return []

    links = []

    for match in LINK_RE.finditer(text):
        target = resolve_markdown_link(path, match.group(1))
        if target is not None:
            links.append(target)

    return links

def collect_reachable() -> set[Path]:
    reachable = set()
    queue = deque()

    if not README.exists():
        raise SystemExit("README.md not found")

    queue.append(README.resolve())

    while queue:
        current = queue.popleft()

        if current in reachable:
            continue

        reachable.add(current)

        for target in outgoing_markdown_links(current):
            if target not in reachable:
                queue.append(target)

    return reachable

def main() -> int:
    reachable = collect_reachable()

    all_docs = {
        path.resolve()
        for path in DOCS.rglob("*.md")
    }

    unreachable = sorted(all_docs - reachable)

    print(f"Reachable markdown files from README.md: {len(reachable)}")
    print(f"Markdown files under docs/: {len(all_docs)}")
    print(f"Unreachable docs markdown files: {len(unreachable)}")

    if unreachable:
        print("")
        print("Files not reachable from README.md:")
        for path in unreachable:
            print(f"- {path.relative_to(ROOT).as_posix()}")
        return 1

    print("")
    print("All docs markdown files are reachable from README.md.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
