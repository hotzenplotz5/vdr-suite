#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST_MAJOR = "Phase 57 - Multi-Site Backend Administration and Permissions"
HISTORICAL_UMBRELLA = "Phase 58 - Frontend and Live Parity"
LATEST_SLICE = "Phase 60.15 - Recording Metadata and Poster Preparation"
NEXT_RUNTIME = "Phase 61 - Suite Metadata Database and External Provider Integration"
ARCHITECTURE_FIRST = "ADR-0042"
ARCHITECTURE_LAST = "ADR-0049"
PARITY_DOC = "parity-audit-and-frontend-gap-roadmap.md"
GAP_MATRIX = "architecture-audit-gap-matrix.md"

REQUIRED_LATEST_MAJOR = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/completed-phases.md",
]

REQUIRED_LATEST_SLICE = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/completed-phases.md",
]

REQUIRED_NEXT_RUNTIME = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/completed-phases.md",
]

REQUIRED_ARCHITECTURE_PACKAGE = [
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/architecture-audit-gap-matrix.md",
    "docs/development/completed-phases-latest.md",
]

REQUIRED_HISTORICAL_UMBRELLA = [
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def require_marker(errors, rel, marker, description):
    path = ROOT / rel
    if not path.exists():
        errors.append(rel + " is missing")
        return
    text = read(rel)
    if marker not in text:
        errors.append(rel + " misses " + description + ": " + marker)


def main():
    errors = []

    for rel in REQUIRED_LATEST_MAJOR:
        require_marker(errors, rel, LATEST_MAJOR, "latest completed major marker")

    for rel in REQUIRED_LATEST_SLICE:
        require_marker(errors, rel, LATEST_SLICE, "latest completed slice marker")

    for rel in REQUIRED_NEXT_RUNTIME:
        require_marker(errors, rel, NEXT_RUNTIME, "next runtime implementation marker")

    for rel in REQUIRED_ARCHITECTURE_PACKAGE:
        require_marker(errors, rel, ARCHITECTURE_FIRST, "architecture package start marker")
        require_marker(errors, rel, ARCHITECTURE_LAST, "architecture package end marker")

    for rel in REQUIRED_HISTORICAL_UMBRELLA:
        require_marker(errors, rel, HISTORICAL_UMBRELLA, "historical Phase 58 umbrella marker")

    parity_path = ROOT / "docs" / "planning" / "parity-audit-and-frontend-gap-roadmap.md"
    if not parity_path.exists():
        errors.append("parity audit planning document is missing")
    else:
        parity_text = parity_path.read_text(encoding="utf-8")
        for marker in ["RESTfulAPI", "Live", "epgsearch", "VDR Core", "Phase 57", "Phase 58"]:
            if marker not in parity_text:
                errors.append("parity audit document misses marker: " + marker)

    for rel in ["docs/NEW-CHAT-HANDOFF.md", "docs/planning/roadmap.md", "docs/planning/index.md"]:
        require_marker(errors, rel, PARITY_DOC, "parity audit link")

    for rel in ["docs/CURRENT.md", "docs/NEW-CHAT-HANDOFF.md", "docs/planning/index.md"]:
        require_marker(errors, rel, GAP_MATRIX, "architecture audit gap matrix link")

    if errors:
        print("Completed phase marker check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Completed phase marker check passed.")
    print("Latest completed major block: " + LATEST_MAJOR)
    print("Latest completed slice: " + LATEST_SLICE)
    print("Immediate architecture package: " + ARCHITECTURE_FIRST + " through " + ARCHITECTURE_LAST)
    print("Next runtime implementation focus: " + NEXT_RUNTIME)
    print("Historical umbrella marker: " + HISTORICAL_UMBRELLA)
    return 0


if __name__ == "__main__":
    sys.exit(main())
