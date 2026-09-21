#!/usr/bin/env python3
from pathlib import Path
import sys

from phase_status_contract import ROOT, load_phase_status

BASE_REQUIRED_LINKS = {
    "README.md": [
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/adr/index.md",
    ],
    "docs/CURRENT.md": [
        "NEW-CHAT-HANDOFF.md",
        "planning/roadmap.md",
        "planning/phase-map.md",
        "planning/golden-user-journeys.md",
        "adr/index.md",
        "development/current-status.md",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "CURRENT.md",
        "planning/roadmap.md",
        "planning/golden-user-journeys.md",
        "adr/index.md",
        "development/completed-phases.md",
    ],
    "docs/development/github-actions-status-handoff.md": [
        "../NEW-CHAT-HANDOFF.md",
    ],
    "docs/planning/roadmap.md": [
        "phase-map.md",
        "golden-user-journeys.md",
        "../CURRENT.md",
    ],
    "docs/planning/phase-map.md": [
        "../CURRENT.md",
        "roadmap.md",
        "golden-user-journeys.md",
    ],
    "docs/planning/index.md": [
        "roadmap.md",
        "phase-map.md",
        "parity-audit-and-frontend-gap-roadmap.md",
        "../CURRENT.md",
        "../NEW-CHAT-HANDOFF.md",
    ],
}

BASE_REQUIRED_TEXT = {
    "README.md": [
        "sole repository authority for volatile operational status",
        "A chat discussion is not a binding VDR-Suite project decision",
    ],
    "docs/CURRENT.md": [
        "## Operational status authority",
    ],
    "docs/NEW-CHAT-HANDOFF.md": [
        "## Current implementation boundary",
    ],
}


def read(rel):
    path = ROOT / rel
    if not path.exists():
        raise FileNotFoundError(rel)
    return path.read_text(encoding="utf-8")


def main():
    errors = []
    try:
        status = load_phase_status()
    except ValueError as exc:
        print("Documentation entrypoint check failed:")
        print("- " + str(exc))
        return 1

    closeout_name = f"phase-{status.latest_completed_number}-closeout.md"
    required_links = {key: list(value) for key, value in BASE_REQUIRED_LINKS.items()}
    required_links["docs/index.md"] = [
        "CURRENT.md",
        "NEW-CHAT-HANDOFF.md",
        "planning/roadmap.md",
        "planning/parity-audit-and-frontend-gap-roadmap.md",
        f"development/{closeout_name}",
    ]
    required_links["docs/CURRENT.md"].append(f"development/{closeout_name}")
    required_links["docs/NEW-CHAT-HANDOFF.md"].append(f"development/{closeout_name}")
    required_links["docs/planning/roadmap.md"].append(f"../development/{closeout_name}")
    required_links["docs/planning/phase-map.md"].append(f"../development/{closeout_name}")

    required_text = {key: list(value) for key, value in BASE_REQUIRED_TEXT.items()}
    required_text["docs/CURRENT.md"].extend(
        [status.latest_completed, status.current_active, status.next_phase]
    )
    required_text["docs/NEW-CHAT-HANDOFF.md"].extend(
        [status.latest_completed, status.next_phase]
    )
    if status.current_active_is_none:
        required_text["docs/NEW-CHAT-HANDOFF.md"].append(status.next_not_started_marker)
    required_text["docs/planning/roadmap.md"] = [
        status.latest_roadmap_heading,
        status.next_roadmap_heading,
    ]
    required_text[status.latest_closeout_rel] = [
        f"**Phase {status.latest_completed_number} is completed.**",
    ]

    for rel, links in required_links.items():
        try:
            text = read(rel)
        except FileNotFoundError:
            errors.append(rel + " is missing")
            continue
        for link in links:
            if link not in text:
                errors.append(rel + " misses link/text: " + link)

    for rel, markers in required_text.items():
        try:
            text = read(rel)
        except FileNotFoundError:
            errors.append(rel + " is missing")
            continue
        for marker in markers:
            if marker not in text:
                errors.append(rel + " misses marker: " + marker)

    if errors:
        print("Documentation entrypoint check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Documentation entrypoint check passed.")
    print("Volatile project status authority: docs/CURRENT.md")
    print("Latest completed numbered runtime phase: " + status.latest_completed)
    print("Current active numbered runtime phase: " + status.current_active)
    print("Next strict numbered runtime phase: " + status.next_phase)
    return 0


if __name__ == "__main__":
    sys.exit(main())
