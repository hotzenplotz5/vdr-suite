#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

LATEST_COMPLETED = "Phase 63 - Backend Agent and Secure Multi-Site Runtime"
ACTIVE_PHASE = "Phase 64 - Timer Intent and Multi-Backend Orchestration"
NEXT_PHASE = "Phase 65 - Streaming Gateway and Media Sessions"
HISTORICAL_UMBRELLA = "Phase 58 - Frontend and Live Parity"
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


def main():
    errors = []

    for rel in STATUS_ENTRYPOINTS:
        require(errors, rel, LATEST_COMPLETED, "latest completed phase marker")
        require(errors, rel, ACTIVE_PHASE, "active phase marker")
        require(errors, rel, NEXT_PHASE, "next numbered phase marker")

    for rel in PLANNING_FILES:
        require(errors, rel, LATEST_COMPLETED, "Phase 63 marker")
        require(errors, rel, ACTIVE_PHASE, "Phase 64 marker")
        require(errors, rel, NEXT_PHASE, "Phase 65 marker")

    # README stays stable and links to the authoritative status/planning files
    # instead of duplicating current phase status.
    require(errors, "README.md", "docs/CURRENT.md", "Current State link")
    require(errors, "README.md", "docs/planning/roadmap.md", "Strict Roadmap link")
    require(errors, "README.md", "A chat discussion is not a binding VDR-Suite project decision", "project-decision rule")

    require(errors, "docs/NEW-CHAT-HANDOFF.md", "phase-62-closeout.md", "Phase 62 closeout link")
    require(errors, "docs/NEW-CHAT-HANDOFF.md", "phase-62-slice-2x-runtime-closeout.md", "Slice 2X closeout link")
    require(errors, "docs/CURRENT.md", HISTORICAL_UMBRELLA, "historical Phase 58 marker")
    require(errors, "docs/development/current-status.md", PHASE62, "Phase 62 historical marker")

    for rel in REQUIRED_HISTORY_FILES:
        if not (ROOT / rel).is_file():
            errors.append(f"closeout/archive file is missing: {rel}")

    stale_active_markers = [
        "Next strict runtime phase:\nPhase 63",
        "Current active runtime slice:\nPhase 63",
        "Current stacked Draft tip:\nPR #169",
    ]
    for rel in STATUS_ENTRYPOINTS:
        text = read(rel)
        for marker in stale_active_markers:
            if marker in text:
                errors.append(f"{rel} still contains stale active marker: {marker}")

    if errors:
        print("Completed phase marker check failed:")
        for item in errors:
            print("- " + item)
        return 1

    print("Completed phase marker check passed.")
    print("Latest completed numbered runtime phase: " + LATEST_COMPLETED)
    print("Current active numbered runtime phase: " + ACTIVE_PHASE)
    print("Next strict numbered runtime phase: " + NEXT_PHASE)
    return 0


if __name__ == "__main__":
    sys.exit(main())
