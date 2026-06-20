#!/usr/bin/env python3
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "README": ROOT / "README.md",
    "current-status": ROOT / "docs/development/current-status.md",
    "project-status-dashboard": ROOT / "docs/project-status-dashboard.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "development-index": ROOT / "docs/development/index.md",
}

COMPLETED_MARKERS = {
    "README": "Latest completed implementation phase",
    "current-status": "Latest completed implementation phase",
    "project-status-dashboard": "Current Major Phase",
    "roadmap": "Completed implementation state",
    "development-index": "Current completed phase",
}

NEXT_MARKERS = {
    "README": "Next major implementation milestone",
    "current-status": "Next Technical Focus",
    "project-status-dashboard": "Next Major Implementation Milestone",
    "roadmap": "Next major implementation milestone",
    "development-index": "Next implementation focus",
}

PHASE_LINE = re.compile(r"Phase\s+\d+(?:\.\d+)?[^\n]*")


def replace_phase_after_marker(path, marker, value):
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue

        for phase_index in range(index, min(len(lines), index + 10)):
            if PHASE_LINE.search(lines[phase_index]):
                lines[phase_index] = PHASE_LINE.sub(value, lines[phase_index], count=1)
                path.write_text("\n".join(lines) + "\n", encoding="utf-8")
                return

    raise SystemExit(f"Could not find phase after marker {marker!r} in {path}")


def completed_phase_heading(completed):
    return "## " + completed


def normalize_summary_lines(summary_lines):
    normalized = []
    for line in summary_lines:
        line = line.strip()
        if not line:
            continue
        if line.startswith("- "):
            normalized.append(line)
        else:
            normalized.append("- " + line)
    return normalized


def insert_completed_phase(completed, summary_lines):
    path = ROOT / "docs/development/completed-phases.md"
    text = path.read_text(encoding="utf-8")

    heading = completed_phase_heading(completed)

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

    summary = "\n".join(normalize_summary_lines(summary_lines))

    entry = (
        heading
        + "\n\n"
        + "Status: Completed.\n\n"
        + "Summary:\n"
        + summary
        + "\n\n"
    )

    preferred_marker = "\n## Detailed Phase History\n\n"
    if preferred_marker in text:
        index = text.find(preferred_marker) + len(preferred_marker)
        text = text[:index] + entry + text[index:]
        path.write_text(text, encoding="utf-8")
        return

    legacy_marker = "\n## Phase "
    index = text.find(legacy_marker)
    if index < 0:
        raise SystemExit("Could not find insertion point in completed-phases.md")

    text = text[: index + 1] + entry + text[index + 1 :]
    path.write_text(text, encoding="utf-8")


def validate_completed_phases_file(next_phase):
    path = ROOT / "docs/development/completed-phases.md"
    text = path.read_text(encoding="utf-8")

    detailed_marker = "## Detailed Phase History"
    if detailed_marker in text:
        text = text.split(detailed_marker, 1)[1]

    heading = "## " + next_phase
    if heading in text:
        raise SystemExit(
            "Refusing to continue: completed-phases.md contains the next phase "
            "as a completed phase heading."
        )


def update(completed, next_phase, summary_lines):
    insert_completed_phase(completed, summary_lines)

    for name, path in FILES.items():
        replace_phase_after_marker(path, COMPLETED_MARKERS[name], completed)
        replace_phase_after_marker(path, NEXT_MARKERS[name], next_phase)

    validate_completed_phases_file(next_phase)


def run_command(command):
    result = subprocess.run(
        command,
        cwd=str(ROOT),
    )
    if result.returncode != 0:
        return result.returncode

    return 0


def run_validation():
    commands = [
        [sys.executable, str(ROOT / "tools/check_phase_consistency.py")],
        [sys.executable, str(ROOT / "tools/check_docs.py")],
        [sys.executable, str(ROOT / "tools/check_doc_indexes.py")],
        [sys.executable, str(ROOT / "tools/check_doc_reachability.py")],
    ]

    for command in commands:
        code = run_command(command)
        if code != 0:
            return code

    return 0


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--completed", required=True)
    parser.add_argument("--next", required=True)
    parser.add_argument(
        "--summary",
        action="append",
        default=[],
        help="Summary line for completed-phases.md. Can be passed multiple times.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate arguments without modifying files.",
    )
    args = parser.parse_args()

    if args.dry_run:
        print("Dry run:")
        print(f"  completed: {args.completed}")
        print(f"  next: {args.next}")
        for line in normalize_summary_lines(args.summary):
            print(f"  summary: {line}")
        return 0

    update(args.completed, args.next, args.summary)

    return run_validation()


if __name__ == "__main__":
    raise SystemExit(main())
