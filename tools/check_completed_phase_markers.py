#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

LATEST_COMPLETED = "Phase 64 - Timer Intent and Multi-Backend Orchestration"
NEXT_PHASE = "Phase 65 - Streaming Gateway and Media Sessions"
NEXT_NOT_STARTED = "Phase 65 has not started"
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
    "docs/development/phase-64-closeout.md",
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


def main():
    errors = []

    for rel in STATUS_ENTRYPOINTS:
        require(errors, rel, LATEST_COMPLETED, "latest completed phase marker")
        require(errors, rel, NEXT_PHASE, "next numbered phase marker")
        require(errors, rel, NEXT_NOT_STARTED, "not-started Phase 65 marker")

    for rel in PLANNING_FILES:
        require(errors, rel, "Phase 64", "Phase 64 marker")
        require(errors, rel, "Phase 65", "Phase 65 marker")

    require(errors, "docs/planning/roadmap.md", "Status: **Completed.**", "completed Phase 64 status")
    require(errors, "docs/planning/roadmap.md", "Status: **Next; not started.**", "next Phase 65 status")

    # README stays stable and links to the authoritative status/planning files
    # instead of duplicating current phase status.
    require(errors, "README.md", "docs/CURRENT.md", "Current State link")
    require(errors, "README.md", "docs/planning/roadmap.md", "Strict Roadmap link")
    require(
        errors,
        "README.md",
        "A chat discussion is not a binding VDR-Suite project decision",
        "project-decision rule",
    )

    # Historical security/Agent foundations remain documented without being
    # mistaken for the latest completed or active phase.
    require(errors, "docs/NEW-CHAT-HANDOFF.md", "phase-62-closeout.md", "Phase 62 closeout link")
    require(errors, "docs/development/current-status.md", PHASE62, "Phase 62 historical marker")
    require(errors, "docs/development/current-status.md", PHASE63, "Phase 63 historical marker")
    require(errors, "docs/development/completed-phases.md", "Phase 58", "historical Phase 58 marker")

    require(
        errors,
        "docs/development/phase-64-closeout.md",
        "PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS",
        "managed fulfillment acceptance marker",
    )
    require(
        errors,
        "docs/development/phase-64-closeout.md",
        "PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS",
        "reassignment/failover acceptance marker",
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
    print("Latest completed numbered runtime phase: " + LATEST_COMPLETED)
    print("Current active numbered runtime phase: none")
    print("Next strict numbered runtime phase: " + NEXT_PHASE + " (not started)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
