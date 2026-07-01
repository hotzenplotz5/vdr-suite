#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST = "Phase 57 - Multi-Site Backend Administration and Permissions"
NEXT = "Phase 58 - Frontend and Live Parity"

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
]

REQUIRED_PLANNED_RANGES = [
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


def p(rel):
    return ROOT / rel


def read(rel):
    return p(rel).read_text(encoding="utf-8")


def error(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    sys.exit(1)


def check():
    phase_map = read("docs/planning/phase-map.md")
    roadmap = read("docs/planning/roadmap.md")

    if "# VDR-Suite Phase Map" not in phase_map:
        error("phase-map.md does not contain the expected title")

    for item in REQUIRED_COMPLETED_RANGES + REQUIRED_PLANNED_RANGES:
        if item not in phase_map:
            error("phase-map.md misses required range: " + item)

    if LATEST not in phase_map:
        error("phase-map.md misses latest completed phase marker")
    if NEXT not in phase_map:
        error("phase-map.md misses next implementation focus marker")

    if "[Phase Map](phase-map.md)" not in roadmap:
        error("roadmap.md does not link to phase-map.md")

    for item in STALE_ROADMAP_MARKERS:
        if item in roadmap:
            error("roadmap.md still contains stale block: " + item)

    for rel in ["README.md", "docs/CURRENT.md"]:
        text = read(rel)
        if LATEST not in text:
            error(rel + " misses latest completed phase marker")
        if NEXT not in text:
            error(rel + " misses next implementation focus marker")

    print("Phase map coverage check passed.")


def main():
    check()
    return 0


if __name__ == "__main__":
    sys.exit(main())
