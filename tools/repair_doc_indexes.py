#!/usr/bin/env python3
from pathlib import Path
import os
import re

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
README = ROOT / "README.md"

SECTION_TITLES = {
    "community": "Community Documentation",
    "introduction": "Introduction",
    "phase-0": "Phase 0 Documentation",
    "status": "Development Status",
    "contribution-guides": "Contribution Guides",
    "phase-11": "Roadmap Phase 11",
    "phase-12": "Roadmap Phase 12",
    "phase-13": "Roadmap Phase 13",
    "phase-14": "Roadmap Phase 14",
    "phase-15": "Roadmap Phase 15",
    "phase-16": "Roadmap Phase 16",
    "phase-17": "Roadmap Phase 17",
    "phase-18": "Roadmap Phase 18",
    "phases": "Roadmap Phases",
}

def rel_link(from_file: Path, target: Path) -> str:
    return Path(os.path.relpath(target, from_file.parent)).as_posix()

def title_from_filename(path: Path) -> str:
    stem = path.stem
    stem = stem.replace("-", " ").replace("_", " ")
    words = []
    for part in stem.split():
        lower = part.lower()
        if lower in {"adr", "api", "vdr", "osd", "ui", "ios"}:
            words.append(lower.upper())
        else:
            words.append(part.capitalize())
    return " ".join(words)

def first_heading(path: Path) -> str:
    try:
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.startswith("# "):
                return line[2:].strip()
    except Exception:
        pass
    return title_from_filename(path)

def section_title(directory: Path) -> str:
    return SECTION_TITLES.get(directory.name, title_from_filename(directory))

def navigation_block(index_file: Path) -> str:
    lines = [
        "## Navigation",
        "",
        f"- [README]({rel_link(index_file, README)})",
        f"- [Documentation Index]({rel_link(index_file, DOCS / 'index.md')})",
        f"- [Project Overview]({rel_link(index_file, DOCS / 'project-overview.md')})",
    ]

    parent_index = index_file.parent.parent / "index.md"
    parent_readme = index_file.parent.parent / "README.md"

    if index_file.parent != DOCS:
        if parent_index.exists():
            lines.append(f"- [Parent Index]({rel_link(index_file, parent_index)})")
        elif parent_readme.exists():
            lines.append(f"- [Parent README]({rel_link(index_file, parent_readme)})")

    lines.extend(["", "---", ""])
    return "\n".join(lines)

def back_block(index_file: Path) -> str:
    lines = ["", "---", "", "## Back", ""]

    parent_index = index_file.parent.parent / "index.md"
    parent_readme = index_file.parent.parent / "README.md"

    if index_file.parent != DOCS:
        if parent_index.exists():
            lines.append(f"- [Back to Parent Index]({rel_link(index_file, parent_index)})")
        elif parent_readme.exists():
            lines.append(f"- [Back to Parent README]({rel_link(index_file, parent_readme)})")

    lines.append(f"- [Back to Documentation Index]({rel_link(index_file, DOCS / 'index.md')})")
    lines.append(f"- [Back to Project Overview]({rel_link(index_file, DOCS / 'project-overview.md')})")
    lines.append(f"- [Back to README]({rel_link(index_file, README)})")
    lines.append("")
    return "\n".join(lines)

def document_links(index_file: Path, directory: Path) -> str:
    md_files = [
        path for path in sorted(directory.glob("*.md"))
        if path.name not in {"index.md", "README.md"}
    ]

    child_indexes = []
    for child in sorted(path for path in directory.iterdir() if path.is_dir()):
        if (child / "index.md").exists():
            child_indexes.append(child / "index.md")
        elif (child / "README.md").exists():
            child_indexes.append(child / "README.md")

    lines = ["## Documents", ""]

    for target in md_files:
        lines.append(f"- [{first_heading(target)}]({rel_link(index_file, target)})")

    if child_indexes:
        if md_files:
            lines.append("")
        lines.append("## Subsections")
        lines.append("")
        for target in child_indexes:
            lines.append(f"- [{first_heading(target)}]({rel_link(index_file, target)})")

    lines.append("")
    return "\n".join(lines)

def should_create_index(directory: Path) -> bool:
    if (directory / "index.md").exists() or (directory / "README.md").exists():
        return False

    has_markdown = any(directory.glob("*.md"))
    has_child_docs = any(child.is_dir() and any(child.glob("*.md")) for child in directory.iterdir())

    return has_markdown or has_child_docs

def create_index(directory: Path) -> None:
    index_file = directory / "index.md"
    content = (
        f"# {section_title(directory)}\n\n"
        + navigation_block(index_file)
        + document_links(index_file, directory)
        + back_block(index_file)
    )

    index_file.write_text(content.rstrip() + "\n", encoding="utf-8")
    print(f"Created {index_file.relative_to(ROOT)}")

def main() -> int:
    created = []

    for directory in sorted(path for path in DOCS.rglob("*") if path.is_dir()):
        if should_create_index(directory):
            create_index(directory)
            created.append(directory / "index.md")

    print("")
    print(f"Created indexes: {len(created)}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
