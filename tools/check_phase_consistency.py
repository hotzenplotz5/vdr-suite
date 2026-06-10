#!/usr/bin/env python3
"""Check phase consistency across key documentation files.

The check intentionally compares documentation state, not git history. Older
commits may legitimately mention older phases, but the active status documents
must agree on the latest completed phase and the next implementation phase.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

KEY_DOCUMENTS = {
    "current-status": ROOT / "docs" / "development" / "current-status.md",
    "project-status-dashboard": ROOT / "docs" / "project-status-dashboard.md",
    "roadmap": ROOT / "docs" / "planning" / "roadmap.md",
    "completed-phases": ROOT / "docs" / "development" / "completed-phases.md",
}

PHASE_RE = re.compile(r"Phase\s+(\d+(?:\.\d+)?[a-z]?)", re.IGNORECASE)
CURRENT_FOCUS_RE = re.compile(r"Current\s+Focus", re.IGNORECASE)
NEXT_STEP_RE = re.compile(r"Next\s+implementation\s+step", re.IGNORECASE)


def phase_key(value: str) -> tuple[int, int, str]:
    match = re.fullmatch(r"(\d+)(?:\.(\d+))?([a-z]?)", value, re.IGNORECASE)
    if not match:
        return (0, 0, value)

    major = int(match.group(1))
    minor = int(match.group(2) or 0)
    suffix = match.group(3).lower()
    return (major, minor, suffix)


def phase_label(value: str | None) -> str:
    return f"Phase {value}" if value else "no phase found"


def phases_in_text(text: str) -> list[str]:
    return [match.group(1) for match in PHASE_RE.finditer(text)]


def latest_phase(text: str) -> str | None:
    phases = phases_in_text(text)
    if not phases:
        return None
    return sorted(phases, key=phase_key)[-1]


def find_phase_after_marker(text: str, marker_re: re.Pattern[str]) -> str | None:
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if not marker_re.search(line):
            continue

        for candidate in lines[index + 1:index + 8]:
            match = PHASE_RE.search(candidate)
            if match:
                return match.group(1)

    return None


def read_documents() -> dict[str, str]:
    documents = {}
    for name, path in KEY_DOCUMENTS.items():
        if not path.exists():
            raise FileNotFoundError(f"Required documentation file missing: {path}")
        documents[name] = path.read_text(encoding="utf-8")
    return documents


def main() -> int:
    errors: list[str] = []
    documents = read_documents()

    latest_by_document = {
        name: latest_phase(text)
        for name, text in documents.items()
    }

    latest_overall = max(
        (phase for phase in latest_by_document.values() if phase is not None),
        key=phase_key,
        default=None,
    )

    if latest_overall is None:
        errors.append("No phase marker found in key documentation files")
    else:
        for name, phase in latest_by_document.items():
            if phase is None:
                errors.append(f"{name} does not mention any phase")
                continue

            if phase_key(phase) < phase_key(latest_overall):
                errors.append(
                    f"{name} is behind latest documented phase: "
                    f"{phase_label(phase)} < {phase_label(latest_overall)}"
                )

    dashboard_focus = find_phase_after_marker(
        documents["project-status-dashboard"],
        CURRENT_FOCUS_RE,
    )
    roadmap_next = find_phase_after_marker(
        documents["roadmap"],
        NEXT_STEP_RE,
    )

    if dashboard_focus and roadmap_next and dashboard_focus != roadmap_next:
        errors.append(
            "project-status-dashboard current focus and roadmap next step disagree: "
            f"{phase_label(dashboard_focus)} != {phase_label(roadmap_next)}"
        )

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("Phase consistency check passed.")
    print(f"Latest documented phase: {phase_label(latest_overall)}")

    for name in sorted(latest_by_document):
        print(f"{name}: {phase_label(latest_by_document[name])}")

    if dashboard_focus:
        print(f"project-status-dashboard current focus: {phase_label(dashboard_focus)}")
    if roadmap_next:
        print(f"roadmap next implementation step: {phase_label(roadmap_next)}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
