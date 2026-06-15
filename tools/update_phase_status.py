#!/usr/bin/env python3

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PHASE_RE = re.compile(r"Phase\s+\d+(?:\.\d+)?[a-z]?\s+-\s+[^\n`]+")

STATUS_FILES = {
    "README": ROOT / "README.md",
    "current-status": ROOT / "docs" / "development" / "current-status.md",
    "project-status-dashboard": ROOT / "docs" / "project-status-dashboard.md",
    "roadmap": ROOT / "docs" / "planning" / "roadmap.md",
    "development-index": ROOT / "docs" / "development" / "index.md",
    "completed-phases": ROOT / "docs" / "development" / "completed-phases.md",
}

PHASE_MARKERS = [
    ("README", "Latest Completed Implementation Phase", "completed"),
    ("README", "Current Implementation Focus", "next"),
    ("current-status", "Latest completed implementation phase", "completed"),
    ("current-status", "Next implementation focus", "next"),
    ("current-status", "Next Technical Focus", "next"),
    ("project-status-dashboard", "Current Major Phase", "completed"),
    ("project-status-dashboard", "Latest Completed Milestone", "completed"),
    ("project-status-dashboard", "Current Focus", "next"),
    ("project-status-dashboard", "Architecture Work In Progress", "next"),
    ("roadmap", "Completed implementation state", "completed"),
    ("roadmap", "Next implementation step", "next"),
    ("development-index", "Current completed phase", "completed"),
    ("development-index", "Next implementation focus", "next"),
]


def read_lines(path):
    return path.read_text(encoding="utf-8").splitlines()


def write_lines(path, lines):
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def replace_phase_after_marker(path, marker, new_phase, apply_changes):
    lines = read_lines(path)
    matches = []

    for marker_index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue

        end = min(len(lines), marker_index + 10)

        for index in range(marker_index, end):
            if PHASE_RE.search(lines[index]):
                matches.append(index)
                break

    if len(matches) != 1:
        raise SystemExit(
            f"{path.relative_to(ROOT)}: marker {marker!r} matched {len(matches)} phase locations"
        )

    index = matches[0]
    old_line = lines[index]
    new_line = PHASE_RE.sub(new_phase, old_line, count=1)

    if old_line == new_line:
        return False

    print(f"{path.relative_to(ROOT)}: {old_line.strip()} -> {new_line.strip()}")

    if apply_changes:
        lines[index] = new_line
        write_lines(path, lines)

    return True


def replace_completed_phases_status(completed, next_text, apply_changes):
    path = STATUS_FILES["completed-phases"]
    text = path.read_text(encoding="utf-8")
    original = text

    text, status_count = re.subn(
        r"Status: Completed through Phase\s+\d+(?:\.\d+)?[a-z]?",
        "Status: Completed through " + completed.split(" - ", 1)[0],
        text,
        count=1,
    )

    if status_count != 1:
        raise SystemExit("completed-phases: expected exactly one completed status marker")

    text, next_count = re.subn(
        r"The next implementation step is [^.]+\.",
        "The next implementation step is " + next_text + ".",
        text,
        count=1,
    )

    if next_count != 1:
        raise SystemExit("completed-phases: expected exactly one next-step sentence")

    if text == original:
        return False

    print("docs/development/completed-phases.md: status/follow-up updated")

    if apply_changes:
        path.write_text(text, encoding="utf-8")

    return True


def derive_next_text(next_phase):
    if " - " not in next_phase:
        raise SystemExit("next phase must use format: Phase X.Y - Title")

    title = next_phase.split(" - ", 1)[1].strip()
    return "the " + title[:1].lower() + title[1:]


def run_verify():
    cmd = [sys.executable, str(ROOT / "tools" / "check_phase_consistency.py")]
    result = subprocess.run(cmd, cwd=ROOT)
    return result.returncode


def update_phase_status(completed, next_phase, next_text, apply_changes):
    changed = False

    for file_key, marker, kind in PHASE_MARKERS:
        path = STATUS_FILES[file_key]
        phase = completed if kind == "completed" else next_phase
        if replace_phase_after_marker(path, marker, phase, apply_changes):
            changed = True

    if replace_completed_phases_status(completed, next_text, apply_changes):
        changed = True

    if not changed:
        print("No phase status changes needed.")

    if apply_changes:
        return run_verify()

    print("Dry run only. Use --apply to write changes.")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Update VDR-Suite phase status markers safely."
    )
    parser.add_argument("--completed", help="Completed phase, for example: Phase 30.4 - Title")
    parser.add_argument("--next", help="Next phase, for example: Phase 30.5 - Title")
    parser.add_argument("--next-text", help="Next step text for completed-phases without a Phase number")
    parser.add_argument("--apply", action="store_true", help="Write changes")
    parser.add_argument("--verify", action="store_true", help="Run phase consistency check only")

    args = parser.parse_args()

    if args.verify:
        return run_verify()

    if not args.completed or not args.next:
        parser.error("--completed and --next are required unless --verify is used")

    next_text = args.next_text or derive_next_text(args.next)

    if PHASE_RE.search(next_text):
        raise SystemExit("next-text must not contain a Phase number")

    return update_phase_status(args.completed, args.next, next_text, args.apply)


if __name__ == "__main__":
    raise SystemExit(main())
