#!/usr/bin/env python3
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "current-status": ROOT / "docs" / "development" / "current-status.md",
    "project-status-dashboard": ROOT / "docs" / "project-status-dashboard.md",
    "roadmap": ROOT / "docs" / "planning" / "roadmap.md",
    "completed-phases": ROOT / "docs" / "development" / "completed-phases.md",
}

PHASE = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)", re.I)


def key(value):
    match = re.match(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.I)
    if not match:
        return (0, 0, value)
    return (int(match.group(1)), int(match.group(2) or 0), match.group(3).lower())


def latest(text):
    values = [m.group(1) for m in PHASE.finditer(text)]
    return sorted(values, key=key)[-1] if values else None


def phase_after(text, marker):
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if marker.lower() not in line.lower():
            continue
        window = "\n".join(lines[index + 1:index + 8])
        match = PHASE.search(window)
        if match:
            return match.group(1)
    return None


def label(value):
    return "Phase " + value if value else "no phase found"


def active_phase(name, text):
    if name == "project-status-dashboard":
        return phase_after(text, "Current Major Phase") or phase_after(text, "Latest Completed Milestone")
    if name == "roadmap":
        return phase_after(text, "Completed implementation state")
    return latest(text)


def main():
    texts = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}
    phases = {name: active_phase(name, text) for name, text in texts.items()}

    newest = max((value for value in phases.values() if value), key=key, default=None)
    errors = []

    if newest is None:
        errors.append("No active phase found in documentation")
    else:
        for name, value in phases.items():
            if not value:
                errors.append(name + " does not mention an active phase")
            elif key(value) < key(newest):
                errors.append(name + " is behind: " + label(value) + " < " + label(newest))

    dashboard_focus = phase_after(texts["project-status-dashboard"], "Current Focus")
    roadmap_next = phase_after(texts["roadmap"], "Next implementation step")

    if dashboard_focus and roadmap_next and dashboard_focus != roadmap_next:
        errors.append(
            "current focus and roadmap next step disagree: "
            + label(dashboard_focus) + " != " + label(roadmap_next)
        )

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Phase consistency check passed.")
    print("Latest active phase: " + label(newest))
    for name in sorted(phases):
        print(name + ": " + label(phases[name]))
    if dashboard_focus:
        print("project-status-dashboard current focus: " + label(dashboard_focus))
    if roadmap_next:
        print("roadmap next implementation step: " + label(roadmap_next))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
