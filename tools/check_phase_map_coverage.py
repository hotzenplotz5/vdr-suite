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
]

STALE_ROADMAP_MARKERS = [
    "### Recording Action Foundation",
    "### EPG Search Foundation",
    "### Phase 55 - Backend Management",
    "### Phase 56 - Backend Capability Matrix",
    "### Phase 59 - Suite Metadata Database",
    "### Phase 60 - Recommendation",
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
        [LATEST_MAJOR, UMBRELLA_TRACK, LATEST_SLICE, NEXT_SLICE, "Phase 61", "Phase 62"],
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
