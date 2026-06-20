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
    "README": "Latest Completed Implementation Phase",
    "current-status": "Latest completed implementation phase",
    "project-status-dashboard": "Current Major Phase",
    "roadmap": "Completed implementation state",
    "development-index": "Current completed phase",
}

NEXT_MARKERS = {
    "README": "Current Implementation Focus",
    "current-status": "Next Technical Focus",
    "project-status-dashboard": "Current Focus",
    "roadmap": "Next implementation step",
    "development-index": "Next implementation focus",
}

PHASE_LINE = re.compile(r"Phase\s+\d+(?:\.\d+)?[^\n]*")


def replace_phase_after_marker(path, marker, value):
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue

        for phase_index in range(index, min(len(lines), index + 8)):
            if PHASE_LINE.search(lines[phase_index]):
                lines[phase_index] = PHASE_LINE.sub(value, lines[phase_index], count=1)
                path.write_text("\n".join(lines) + "\n", encoding="utf-8")
                return

    raise SystemExit(f"Could not find phase after marker {marker!r} in {path}")


def replace_readme_line(path, prefix, value):
    text = path.read_text(encoding="utf-8")
    pattern = re.compile(r"^" + re.escape(prefix) + r".*$", re.MULTILINE)
    text, count = pattern.subn(prefix + value, text, count=1)
    if count != 1:
        raise SystemExit(f"Could not find README line prefix {prefix!r}")
    path.write_text(text, encoding="utf-8")


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

    marker = "\n## Phase "
    index = text.find(marker)
    if index < 0:
        raise SystemExit("Could not find insertion point in completed-phases.md")

    text = text[: index + 1] + entry + text[index + 1 :]
    path.write_text(text, encoding="utf-8")


def validate_completed_phases_file(next_phase):
    path = ROOT / "docs/development/completed-phases.md"
    text = path.read_text(encoding="utf-8")
    if next_phase in text:
        raise SystemExit(
            "Refusing to continue: completed-phases.md contains the next phase. "
            "check_phase_consistency.py treats the highest phase in that file as completed."
        )


def update(completed, next_phase, summary_lines):
    insert_completed_phase(completed, summary_lines)

    replace_readme_line(
        FILES["README"],
        "Latest Completed Implementation Phase: ",
        completed,
    )
    replace_readme_line(
        FILES["README"],
        "Current Implementation Focus: ",
        next_phase,
    )

    for name, path in FILES.items():
        if name == "README":
            continue
        replace_phase_after_marker(path, COMPLETED_MARKERS[name], completed)
        replace_phase_after_marker(path, NEXT_MARKERS[name], next_phase)

    validate_completed_phases_file(next_phase)


def run_checker():
    result = subprocess.run(
        [sys.executable, str(ROOT / "tools/check_phase_consistency.py")],
        cwd=str(ROOT),
    )
    return result.returncode


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
    args = parser.parse_args()

    update(args.completed, args.next, args.summary)

    return run_checker()


if __name__ == "__main__":
    raise SystemExit(main())
