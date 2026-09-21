#!/usr/bin/env python3
from pathlib import Path
import sys

from phase_status_contract import ROOT, load_phase_status

PHASE63 = "Phase 63"
PHASE62 = "Phase 62 - Identity, RBAC and Accountability Foundation"

STATUS_ENTRYPOINTS = [
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/development/current-status.md",
]

PLANNING_FILES = [
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
]

REQUIRED_HISTORY_FILES = [
    "docs/development/phase-61-metadata-genre-performance-closeout.md",
    "docs/development/post-phase-61-platform-runtime-closeout.md",
    "docs/development/completed-phases/phase-61.md",
    "docs/development/phase-62-closeout.md",
    "docs/development/phase-62-slice-2x-runtime-closeout.md",
    "docs/development/completed-phases.md",
]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def require(errors, rel, marker, description):
    path = ROOT / rel
    if not path.is_file():
        errors.append(f"{rel} is missing")
        return
    if marker not in read(rel):
        errors.append(f"{rel} misses {description}: {marker}")


def forbid(errors, rel, marker, description):
    path = ROOT / rel
    if not path.is_file():
        errors.append(f"{rel} is missing")
        return
    if marker in read(rel):
        errors.append(f"{rel} still contains {description}: {marker}")


def roadmap_section(text, heading):
    marker = "## " + heading
    start = text.find(marker)
    if start < 0:
        return ""
    next_heading = text.find("\n## ", start + len(marker))
    return text[start:] if next_heading < 0 else text[start:next_heading]


def main():
    errors = []
    try:
        status = load_phase_status()
    except ValueError as exc:
        print("Completed phase marker check failed:")
        print("- " + str(exc))
        return 1

    for rel in STATUS_ENTRYPOINTS:
        require(errors, rel, status.latest_completed, "latest completed phase marker")
        require(errors, rel, status.next_phase, "next numbered phase marker")
        if status.current_active_is_none:
            require(
                errors,
                rel,
                status.next_not_started_marker,
                "next-phase not-started marker",
            )

    for rel in PLANNING_FILES:
        require(errors, rel, f"Phase {status.latest_completed_number}", "latest phase marker")
        require(errors, rel, f"Phase {status.next_phase_number}", "next phase marker")

    roadmap_rel = "docs/planning/roadmap.md"
    roadmap = read(roadmap_rel) if (ROOT / roadmap_rel).is_file() else ""
    completed_section = roadmap_section(roadmap, status.latest_roadmap_heading)
    next_section = roadmap_section(roadmap, status.next_roadmap_heading)
    if "Status: **Completed.**" not in completed_section:
        errors.append(
            f"{roadmap_rel} latest phase section is not marked Completed: {status.latest_roadmap_heading}"
        )
    if status.current_active_is_none and "Status: **Next; not started.**" not in next_section:
        errors.append(
            f"{roadmap_rel} next phase section is not marked Next; not started: {status.next_roadmap_heading}"
        )

    require(errors, "README.md", "docs/CURRENT.md", "Current State link")
    require(errors, "README.md", "docs/planning/roadmap.md", "Strict Roadmap link")
    require(
        errors,
        "README.md",
        "A chat discussion is not a binding VDR-Suite project decision",
        "project-decision rule",
    )

    # Historical foundations stay guarded independently of volatile status.
    require(errors, "docs/NEW-CHAT-HANDOFF.md", "phase-62-closeout.md", "Phase 62 closeout link")
    require(errors, "docs/development/current-status.md", PHASE62, "Phase 62 historical marker")
    require(errors, "docs/development/current-status.md", PHASE63, "Phase 63 historical marker")
    require(errors, "docs/development/completed-phases.md", "Phase 58", "historical Phase 58 marker")

    latest_closeout = status.latest_closeout_rel
    if not status.latest_closeout_path.is_file():
        errors.append(f"latest phase closeout is missing: {latest_closeout}")
    else:
        require(
            errors,
            latest_closeout,
            f"**Phase {status.latest_completed_number} is completed.**",
            "latest completed phase closeout marker",
        )

    for rel in REQUIRED_HISTORY_FILES:
        if not (ROOT / rel).is_file():
            errors.append(f"closeout/archive file is missing: {rel}")

    stale_active_markers = [
        "Next strict runtime phase:\nPhase 63",
        "Current active runtime slice:\nPhase 63",
        "Current stacked Draft tip:\nPR #169",
        "planning hold after the PR-#190 checkpoint",
        "No Phase-64 successor implementation is currently authorized",
    ]
    for rel in STATUS_ENTRYPOINTS:
        for marker in stale_active_markers:
            forbid(errors, rel, marker, "stale active marker")

    if errors:
        print("Completed phase marker check failed:")
        for item in errors:
            print("- " + item)
        return 1

    print("Completed phase marker check passed.")
    print("Latest completed numbered runtime phase: " + status.latest_completed)
    print("Current active numbered runtime phase: " + status.current_active)
    print("Next strict numbered runtime phase: " + status.next_phase)
    return 0


if __name__ == "__main__":
    sys.exit(main())
