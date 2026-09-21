#!/usr/bin/env python3
from pathlib import Path
from urllib.parse import unquote
import re
import sys
from collections import deque

ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"
DOCS = ROOT / "docs"

LINK_RE = re.compile(r"(?<!!)\[[^\]]+\]\(([^)]+)\)")
SKIP_PREFIXES = ("http://", "https://", "mailto:", "tel:")

CANONICAL_DOCS = [
    "docs/index.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/project-principles.md",
    "docs/project-overview.md",
    "docs/project-status-dashboard.md",
    "docs/development/index.md",
    "docs/development/current-status.md",
    "docs/development/current-architecture-state.md",
    "docs/development/completed-phases.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/phase-61-metadata-genre-performance-closeout.md",
    "docs/development/post-phase-61-platform-runtime-closeout.md",
    "docs/development/github-actions-status-handoff.md",
    "docs/development/person-api.md",
    "docs/development/web-client-api-contract-snapshot.md",
    "docs/planning/index.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/planning/domain-dependency-map.md",
    "docs/planning/implementation-dependency-map.md",
    "docs/planning/architecture-audit-gap-matrix.md",
    "docs/planning/parity-audit-and-frontend-gap-roadmap.md",
    "docs/planning/tvscraper-recording-metadata-roadmap.md",
    "docs/architecture/index.md",
    "docs/architecture/target-platform-architecture.md",
    "docs/architecture/metadata-genre-browser.md",
    "docs/architecture/global-search.md",
    "docs/architecture/live-remote-osd-contract.md",
    "docs/architecture/restfulapi-integration.md",
    "docs/adr/index.md",
]


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
    return target.lower().startswith(SKIP_PREFIXES)


def resolve_markdown_link(source: Path, raw_target: str) -> Path | None:
    target = strip_link_target(raw_target)
    if not target or is_external(target) or target.startswith("#"):
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
        for name in ("README.md", "index.md"):
            index = candidate / name
            if index.exists():
                candidate = index
                break

    if candidate.suffix.lower() != ".md" or not candidate.exists():
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
    queue = deque([README.resolve()])
    while queue:
        current = queue.popleft()
        if current in reachable:
            continue
        reachable.add(current)
        for target in outgoing_markdown_links(current):
            if target not in reachable:
                queue.append(target)
    return reachable


def required_docs() -> set[Path]:
    result = {(ROOT / rel).resolve() for rel in CANONICAL_DOCS}
    for path in DOCS.rglob("*.md"):
        rel_parts = path.relative_to(DOCS).parts
        if "history" in rel_parts or "completed-phases" in rel_parts:
            result.add(path.resolve())
    return result


def main() -> int:
    if not README.exists():
        print("README.md not found", file=sys.stderr)
        return 2

    reachable = collect_reachable()
    required = required_docs()
    missing_files = sorted(path for path in required if not path.exists())
    unreachable_required = sorted((required - set(missing_files)) - reachable)

    all_docs = {path.resolve() for path in DOCS.rglob("*.md")}
    informational = sorted((all_docs - reachable) - set(unreachable_required))

    print(f"Reachable Markdown files from README.md: {len(reachable)}")
    print(f"Required current/archive Markdown files: {len(required)}")
    print(f"Unreachable required files: {len(unreachable_required)}")
    print(f"Unlinked legacy leaf files (informational): {len(informational)}")

    if missing_files or unreachable_required:
        if missing_files:
            print("\nMissing required files:")
            for path in missing_files:
                print(f"- {path.relative_to(ROOT).as_posix()}")
        if unreachable_required:
            print("\nRequired files not reachable from README.md:")
            for path in unreachable_required:
                print(f"- {path.relative_to(ROOT).as_posix()}")
        return 1

    print("\nAll canonical current and archive documents are reachable from README.md.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
