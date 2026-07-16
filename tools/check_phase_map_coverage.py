#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST_MAJOR = "Phase 57 - Multi-Site Backend Administration and Permissions"
UMBRELLA_TRACK = "Phase 58 - Frontend and Live Parity"
LATEST_SLICE = "Phase 60.14k - Recording Detail UX Polish"
NEXT_SLICE = "Phase 60.15 - Recording Metadata and Poster Preparation"

REQUIRED_COMPLETED_RANGES = [
    "Phase 1.x-7.x",
    "Phase 8.x",
    "Phase 9.x-29.x",
    "Phase 30.x-36.x",
    "Phase 37.x-44.x",
    "Phase 45.x",
    "Phase 46.x",
    "Phase 47.x-49.x",
    "Phase 50.0-50.50",
    "Phase 51.x",
    "Phase 52.x",
    "Phase 53.x",
    "Phase 54.x",
    "Phase 55.0-55.4e",
    "Phase 55.5a-55.5n",
    "Phase 55.5o",
    "Phase 55.6",
    "Phase 56",
    "Phase 57",
    "Phase 58.0-58.90b",
    "Phase 59.00-59.15e",
    "Phase 60.1-60.14k",
]

REQUIRED_CURRENT_AND_PLANNED = [
    "Phase 60.15",
    "Phase 61",
    "Phase 62",
    "Phase 63",
    "Phase 64",
    "Phase 65",
    "Phase 66",
    "Phase 67",
    "Phase 68",
]

REQUIRED_ROADMAP_ORDER = [
    "Step 1 - Complete the Architecture Contract Package",
    "Step 2 - Phase 60.15: Recording Metadata and Poster Preparation",
    "Step 3 - Phase 61: Suite Metadata Database and External Providers",
    "Step 4 - Phase 62: Identity, RBAC and Audit Foundation",
    "Step 5 - Phase 63: Backend Agent and Secure Multi-Site Runtime",
    "Step 6 - Phase 64: Timer Intent and Multi-Backend Orchestration",
    "Step 7 - Phase 65: Streaming Gateway and Media Sessions",
    "Step 8 - Phase 66: Legacy OSD Compatibility Bridge",
    "Step 9 - Phase 67: Public API and Client Compatibility Hardening",
    "Step 10 - Phase 68: Recommendation and Content Knowledge Graph",
]

STALE_ROADMAP_MARKERS = [
    "### Recording Action Foundation",
    "### EPG Search Foundation",
    "### Phase 55 - Backend Management",
    "### Phase 56 - Backend Capability Matrix",
    "### Phase 59 - Suite Metadata Database",
    "### Phase 60 - Recommendation",
    "Phase 62 - Recommendation and Content Knowledge Graph",
    "runtime milestone number not yet assigned",
]


def p(rel):
    return ROOT / rel


def read(rel):
    return p(rel).read_text(encoding="utf-8")


def error(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    sys.exit(1)


def require_markers(text, rel, markers):
    for marker in markers:
        if marker not in text:
            error(rel + " misses required marker: " + marker)


def require_order(text, rel, markers):
    positions = []
    for marker in markers:
        position = text.find(marker)
        if position < 0:
            error(rel + " misses ordered marker: " + marker)
        positions.append(position)

    if positions != sorted(positions):
        error(rel + " does not preserve the required roadmap order")


def check():
    phase_map = read("docs/planning/phase-map.md")
    roadmap = read("docs/planning/roadmap.md")

    if "# VDR-Suite Phase Map" not in phase_map:
        error("phase-map.md does not contain the expected title")

    for item in REQUIRED_COMPLETED_RANGES + REQUIRED_CURRENT_AND_PLANNED:
        if item not in phase_map:
            error("phase-map.md misses required range: " + item)

    require_markers(
        phase_map,
        "docs/planning/phase-map.md",
        [LATEST_MAJOR, UMBRELLA_TRACK, LATEST_SLICE, NEXT_SLICE],
    )

    if "[Phase Map](phase-map.md)" not in roadmap:
        error("roadmap.md does not link to phase-map.md")

    require_markers(
        roadmap,
        "docs/planning/roadmap.md",
        [LATEST_MAJOR, UMBRELLA_TRACK, LATEST_SLICE, NEXT_SLICE]
        + REQUIRED_CURRENT_AND_PLANNED,
    )
    require_order(
        roadmap,
        "docs/planning/roadmap.md",
        REQUIRED_ROADMAP_ORDER,
    )

    for item in STALE_ROADMAP_MARKERS:
        if item in roadmap:
            error("roadmap.md still contains stale block: " + item)

    for rel in ["README.md", "docs/CURRENT.md", "docs/NEW-CHAT-HANDOFF.md"]:
        text = read(rel)
        require_markers(
            text,
            rel,
            [LATEST_MAJOR, UMBRELLA_TRACK, LATEST_SLICE, NEXT_SLICE],
        )

    print("Phase map coverage check passed.")


def main():
    check()
    return 0


if __name__ == "__main__":
    sys.exit(main())
