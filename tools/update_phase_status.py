#!/usr/bin/env python3
"""Update the canonical numbered runtime-phase status and its narrative mirrors.

The volatile authority is always docs/CURRENT.md. This helper updates the three
canonical status values there first, then synchronizes only the explicitly
labelled mirror lines/blocks in the handoff, status, roadmap and phase map.
It intentionally does not invent closeout evidence, roadmap content or runtime
authorization; the normal guards must still validate those separately.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

from phase_status_contract import (
    ACTIVE_LABEL,
    CURRENT,
    LATEST_LABEL,
    NEXT_LABEL,
    ROOT,
    parse_phase_status,
    replace_current_status_values,
)

BLOCK_MIRRORS = [
    ROOT / "docs/planning/roadmap.md",
    ROOT / "docs/planning/phase-map.md",
]
INLINE_MIRRORS = [
    ROOT / "docs/NEW-CHAT-HANDOFF.md",
    ROOT / "docs/development/current-status.md",
]


def normalize_summary_lines(summary_lines):
    normalized = []
    for line in summary_lines:
        line = line.strip()
        if not line:
            continue
        normalized.append(line if line.startswith("- ") else "- " + line)
    return normalized


def insert_completed_phase(completed, summary_lines):
    path = ROOT / "docs/development/completed-phases.md"
    text = path.read_text(encoding="utf-8")
    heading = "## " + completed

    if heading in text:
        return
    if not summary_lines:
        raise SystemExit(
            "completed-phases.md does not contain the completed phase yet. "
            "Pass at least one --summary line so the tool can create the entry."
        )
    if completed in text:
        raise SystemExit(
            "Refusing ambiguous update: completed phase appears in completed-phases.md "
            "but not as the expected phase heading."
        )

    entry = (
        heading
        + "\n\nStatus: Completed.\n\nSummary:\n"
        + "\n".join(normalize_summary_lines(summary_lines))
        + "\n\n"
    )

    preferred_marker = "\n## Detailed Phase History\n\n"
    if preferred_marker in text:
        index = text.find(preferred_marker) + len(preferred_marker)
        path.write_text(text[:index] + entry + text[index:], encoding="utf-8")
        return

    legacy_marker = "\n## Phase "
    index = text.find(legacy_marker)
    if index < 0:
        raise SystemExit("Could not find insertion point in completed-phases.md")
    path.write_text(text[: index + 1] + entry + text[index + 1 :], encoding="utf-8")


def validate_completed_phases_file(next_phase):
    path = ROOT / "docs/development/completed-phases.md"
    text = path.read_text(encoding="utf-8")
    detailed_marker = "## Detailed Phase History"
    if detailed_marker in text:
        text = text.split(detailed_marker, 1)[1]
    if "## " + next_phase in text:
        raise SystemExit(
            "Refusing to continue: completed-phases.md contains the next phase "
            "as a completed phase heading."
        )


def replace_block_status(path: Path, completed: str, active: str, next_phase: str):
    text = path.read_text(encoding="utf-8")
    updated = replace_current_status_values(
        text,
        completed=completed,
        active=active,
        next_phase=next_phase,
    )
    path.write_text(updated, encoding="utf-8")


def replace_inline_status_value(text: str, label: str, value: str) -> str:
    pattern = re.compile(
        rf"(?m)^(?P<prefix>\s*-\s*)?{re.escape(label)}\s*\*\*[^*\n]+\*\*\.\s*$"
    )
    matches = list(pattern.finditer(text))
    if len(matches) != 1:
        raise SystemExit(
            f"Expected exactly one inline status marker {label!r}; found {len(matches)}"
        )
    prefix = matches[0].group("prefix") or ""
    replacement = f"{prefix}{label} **{value}**."
    return pattern.sub(replacement, text, count=1)


def replace_inline_status(path: Path, completed: str, active: str, next_phase: str):
    text = path.read_text(encoding="utf-8")
    mirror_active = active.replace("none - ", "none; ", 1)
    text = replace_inline_status_value(text, LATEST_LABEL, completed)
    text = replace_inline_status_value(text, ACTIVE_LABEL, mirror_active)
    text = replace_inline_status_value(text, NEXT_LABEL, next_phase)
    path.write_text(text, encoding="utf-8")


def roadmap_heading(phase: str) -> str:
    return phase.replace(" - ", " — ", 1)


def set_roadmap_section_status(text: str, heading: str, expected: str) -> str:
    marker = "## " + heading
    start = text.find(marker)
    if start < 0:
        raise SystemExit(f"Roadmap is missing numbered phase section: {heading}")
    end = text.find("\n## ", start + len(marker))
    if end < 0:
        end = len(text)
    section = text[start:end]
    pattern = re.compile(r"Status: \*\*[^*\n]+\.\*\*")
    matches = list(pattern.finditer(section))
    if not matches:
        raise SystemExit(f"Roadmap phase section has no status line: {heading}")
    section = pattern.sub(f"Status: **{expected}.**", section, count=1)
    return text[:start] + section + text[end:]


def update(completed, active, next_phase, summary_lines):
    # Validate the requested tuple before touching files.
    synthetic = (
        f"{LATEST_LABEL}\n{completed}\n\n"
        f"{ACTIVE_LABEL}\n{active}\n\n"
        f"{NEXT_LABEL}\n{next_phase}\n"
    )
    parse_phase_status(synthetic)

    insert_completed_phase(completed, summary_lines)

    current_text = CURRENT.read_text(encoding="utf-8")
    CURRENT.write_text(
        replace_current_status_values(
            current_text,
            completed=completed,
            active=active,
            next_phase=next_phase,
        ),
        encoding="utf-8",
    )

    for path in BLOCK_MIRRORS:
        replace_block_status(path, completed, active, next_phase)
    for path in INLINE_MIRRORS:
        replace_inline_status(path, completed, active, next_phase)

    roadmap = BLOCK_MIRRORS[0]
    roadmap_text = roadmap.read_text(encoding="utf-8")
    roadmap_text = set_roadmap_section_status(
        roadmap_text, roadmap_heading(completed), "Completed"
    )
    if active.startswith("none - "):
        roadmap_text = set_roadmap_section_status(
            roadmap_text, roadmap_heading(next_phase), "Next; not started"
        )
    roadmap.write_text(roadmap_text, encoding="utf-8")

    validate_completed_phases_file(next_phase)


def run_validation():
    commands = [
        [sys.executable, str(ROOT / "tools/check_phase_consistency.py")],
        [sys.executable, str(ROOT / "tools/check_completed_phase_markers.py")],
        [sys.executable, str(ROOT / "tools/check_phase_map_coverage.py")],
        [sys.executable, str(ROOT / "tools/check_doc_entrypoints.py")],
        [sys.executable, str(ROOT / "tools/check_docs.py")],
        [sys.executable, str(ROOT / "tools/check_doc_indexes.py")],
        [sys.executable, str(ROOT / "tools/check_doc_reachability.py")],
    ]
    for command in commands:
        result = subprocess.run(command, cwd=str(ROOT))
        if result.returncode != 0:
            return result.returncode
    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--completed", required=True)
    parser.add_argument("--next", required=True)
    parser.add_argument(
        "--active",
        help=(
            "Canonical active value. Defaults to 'none - Phase N has not started' "
            "for the --next phase."
        ),
    )
    parser.add_argument(
        "--summary",
        action="append",
        default=[],
        help="Summary line for completed-phases.md. Can be passed multiple times.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate the requested phase tuple without modifying files.",
    )
    args = parser.parse_args()

    next_match = re.match(r"^Phase\s+(\d+)\s+-\s+", args.next)
    if not next_match:
        raise SystemExit("--next must use canonical 'Phase N - Title' spelling")
    active = args.active or f"none - Phase {next_match.group(1)} has not started"

    synthetic = (
        f"{LATEST_LABEL}\n{args.completed}\n\n"
        f"{ACTIVE_LABEL}\n{active}\n\n"
        f"{NEXT_LABEL}\n{args.next}\n"
    )
    parse_phase_status(synthetic)

    if args.dry_run:
        print("Dry run:")
        print(f"  completed: {args.completed}")
        print(f"  active: {active}")
        print(f"  next: {args.next}")
        for line in normalize_summary_lines(args.summary):
            print(f"  summary: {line}")
        return 0

    update(args.completed, active, args.next, args.summary)
    return run_validation()


if __name__ == "__main__":
    raise SystemExit(main())
