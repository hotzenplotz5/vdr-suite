#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "README": ROOT / "README.md",
    "current-status": ROOT / "docs" / "development" / "current-status.md",
    "project-status-dashboard": ROOT / "docs" / "project-status-dashboard.md",
    "roadmap": ROOT / "docs" / "planning" / "roadmap.md",
    "completed-phases": ROOT / "docs" / "development" / "completed-phases.md",
    "development-index": ROOT / "docs" / "development" / "index.md",
}

PHASE = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)", re.I)
PHASE_HEADING = re.compile(r"^##\s+Phase\s+(\d+(?:\.\d+)?[a-z]?)\b", re.I | re.M)


def key(value):
    match = re.match(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.I)
    if not match:
        return (0, 0, value)
    return (int(match.group(1)), int(match.group(2) or 0), match.group(3).lower())


def latest(text):
    values = [m.group(1) for m in PHASE.finditer(text)]
    return sorted(values, key=key)[-1] if values else None


def latest_completed_heading(text):
    values = [m.group(1) for m in PHASE_HEADING.finditer(text)]
    return sorted(values, key=key)[-1] if values else None


def phase_after(text, marker):
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue

        match = PHASE.search(line)
        if match:
            return match.group(1)

        window = "\n".join(lines[index + 1:index + 10])
        match = PHASE.search(window)
        if match:
            return match.group(1)
    return None


def first_phase_after_any(text, markers):
    for marker in markers:
        value = phase_after(text, marker)
        if value:
            return value
    return None


def label(value):
    return "Phase " + value if value else "no phase found"


def completed_phase(name, text):
    if name == "README":
        return first_phase_after_any(text, [
            "Latest completed implementation phase",
            "Latest Completed Implementation Phase",
        ])
    if name == "current-status":
        return first_phase_after_any(text, [
            "Latest completed implementation phase",
        ])
    if name == "project-status-dashboard":
        return first_phase_after_any(text, [
            "Current Major Phase",
            "Latest completed implementation phase",
            "Latest Completed Milestone",
            "Latest Completed Milestones",
        ])
    if name == "roadmap":
        return first_phase_after_any(text, [
            "Completed implementation state",
        ])
    if name == "development-index":
        return first_phase_after_any(text, [
            "Current completed phase",
        ])
    if name == "completed-phases":
        return latest_completed_heading(text)
    return latest(text)


def next_phase(name, text):
    if name == "README":
        return first_phase_after_any(text, [
            "Next major implementation milestone",
            "Next Major Implementation Milestone",
            "Current Implementation Focus",
        ])
    if name == "current-status":
        return first_phase_after_any(text, [
            "Next Technical Focus",
            "Next major implementation milestone",
        ])
    if name == "project-status-dashboard":
        return first_phase_after_any(text, [
            "Next Major Implementation Milestone",
            "Current Focus",
        ])
    if name == "roadmap":
        return first_phase_after_any(text, [
            "Next major implementation milestone",
            "Next implementation step",
        ])
    if name == "development-index":
        return first_phase_after_any(text, [
            "Next implementation focus",
        ])
    return None


def read_files():
    texts = {}
    for name, path in FILES.items():
        if not path.exists():
            raise FileNotFoundError(str(path))
        texts[name] = path.read_text(encoding="utf-8")
    return texts


def main():
    texts = read_files()
    completed = {name: completed_phase(name, text) for name, text in texts.items()}
    planned = {name: next_phase(name, text) for name, text in texts.items()}

    newest = max((value for value in completed.values() if value), key=key, default=None)
    expected_next = max((value for value in planned.values() if value), key=key, default=None)

    errors = []

    if newest is None:
        errors.append("No completed phase found in documentation")
    else:
        for name, value in completed.items():
            if not value:
                errors.append(name + " does not mention a completed phase")
            elif value != newest:
                errors.append(name + " completed phase differs: " + label(value) + " != " + label(newest))

    if expected_next is not None:
        for name, value in planned.items():
            if value and value != expected_next:
                errors.append(name + " next phase differs: " + label(value) + " != " + label(expected_next))

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Phase consistency check passed.")
    print("Latest completed phase: " + label(newest))
    for name in sorted(completed):
        print(name + ": " + label(completed[name]))
    if expected_next:
        print("Next implementation focus: " + label(expected_next))
        for name in sorted(planned):
            if planned[name]:
                print(name + " next: " + label(planned[name]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
