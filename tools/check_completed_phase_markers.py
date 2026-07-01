#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST = "Phase 57 - Multi-Site Backend Administration and Permissions"
NEXT = "Phase 58 - Frontend and Live Parity"
PHASE_58 = "Phase 58"
PARITY_DOC = "parity-audit-and-frontend-gap-roadmap.md"

REQUIRED_LATEST = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/completed-phases.md",
]

REQUIRED_NEXT = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
]

REQUIRED_PHASE58 = [
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def require_marker(errors, rel, marker, description):
    path = ROOT / rel
    if not path.exists():
        errors.append(rel + " is missing")
        return
    if marker not in read(rel):
        errors.append(rel + " misses " + description + ": " + marker)


def main():
    errors = []

    for rel in REQUIRED_LATEST:
        require_marker(errors, rel, LATEST, "latest completed marker")

    for rel in REQUIRED_NEXT:
        require_marker(errors, rel, NEXT, "next implementation marker")

    for rel in REQUIRED_PHASE58:
        require_marker(errors, rel, PHASE_58, "Phase 58 planning marker")

    parity_path = ROOT / "docs" / "planning" / "parity-audit-and-frontend-gap-roadmap.md"
    if not parity_path.exists():
        errors.append("parity audit planning document is missing")
    elif PARITY_DOC not in read("docs/NEW-CHAT-HANDOFF.md"):
        errors.append("handoff misses parity audit link")

    for rel in ["docs/planning/roadmap.md", "docs/planning/index.md"]:
        require_marker(errors, rel, PARITY_DOC, "parity audit link")

    if errors:
        print("Completed phase marker check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Completed phase marker check passed.")
    print("Latest completed phase: " + LATEST)
    print("Next implementation focus: " + NEXT)
    print("Planned frontend block marker: " + PHASE_58)
    return 0


if __name__ == "__main__":
    sys.exit(main())
