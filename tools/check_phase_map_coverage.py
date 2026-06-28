#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST = "Phase 55.5o - Phase map and roadmap simplification"
NEXT = "Phase 55.6 - Recording operations audit and safety policy"

REQUIRED = [
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
    "Phase 58",
    "Phase 59",
    "Phase 60",
]

STALE_ROADMAP_MARKERS = [
    "### Recording Action Foundation",
    "### EPG Search Foundation",
    "### Phase 55 - Backend Management",
    "### Phase 56 - Backend Capability Matrix",
]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def error(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    sys.exit(1)


def require_contains(haystack, needle, place):
    if needle not in haystack:
        error(place + " misses: " + needle)


def main():
    phase_map = read("docs/planning/phase-map.md")
    roadmap = read("docs/planning/roadmap.md")

    require_contains(phase_map, "# VDR-Suite Phase Map", "phase-map.md")
    for item in REQUIRED:
        require_contains(phase_map, item, "phase-map.md")

    require_contains(roadmap, "[Phase Map](phase-map.md)", "roadmap.md")
    for item in STALE_ROADMAP_MARKERS:
        if item in roadmap:
            error("roadmap.md still contains stale block: " + item)

    latest_docs = [
        "README.md",
        "docs/development/current-status.md",
        "docs/development/index.md",
        "docs/project-status-dashboard.md",
        "docs/development/completed-phases.md",
        "docs/planning/roadmap.md",
    ]
    for rel in latest_docs:
        require_contains(read(rel), LATEST, rel)

    next_docs = [
        "README.md",
        "docs/development/current-status.md",
        "docs/development/index.md",
        "docs/project-status-dashboard.md",
        "docs/planning/roadmap.md",
    ]
    for rel in next_docs:
        require_contains(read(rel), NEXT, rel)

    print("Phase map coverage check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
