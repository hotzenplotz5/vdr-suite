#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST_PHASE = "Phase 61 - Suite Metadata and Genre Platform"
HISTORICAL_UMBRELLA = "Phase 58 - Frontend and Live Parity"
LATEST_HARDENING = "Post-Phase 61 Performance Hardening (B1-B4)"
REMOTE_FEATURE = "VDR Remote and Live Overlay hardening (#110)"
SEARCH_FEATURE = "Backend-scoped Global Search (#111)"
NEXT_RUNTIME = "Phase 62 - Identity, RBAC and Accountability Foundation"
ARCHITECTURE_FIRST = "ADR-0042"
ARCHITECTURE_LAST = "ADR-0049"
CLOSEOUT_PHASE61 = "phase-61-metadata-genre-performance-closeout.md"
CLOSEOUT_PLATFORM = "post-phase-61-platform-runtime-closeout.md"
PARITY_DOC = "parity-audit-and-frontend-gap-roadmap.md"
GAP_MATRIX = "architecture-audit-gap-matrix.md"

CANONICAL_STATUS_FILES = [
    "README.md",
    "docs/CURRENT.md",
    "docs/NEW-CHAT-HANDOFF.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/current-status.md",
    "docs/development/completed-phases-latest.md",
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

    for rel in CANONICAL_STATUS_FILES:
        require(errors, rel, LATEST_PHASE, "latest completed phase marker")
        require(errors, rel, LATEST_HARDENING, "latest hardening marker")
        require(errors, rel, NEXT_RUNTIME, "next runtime marker")

    for rel in [
        "README.md",
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/planning/phase-map.md",
        "docs/development/completed-phases-latest.md",
        "docs/development/completed-phases.md",
    ]:
        require(errors, rel, REMOTE_FEATURE, "completed Remote marker")
        require(errors, rel, SEARCH_FEATURE, "completed Global Search marker")

    for rel in [
        "README.md",
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/planning/phase-map.md",
    ]:
        require(errors, rel, HISTORICAL_UMBRELLA, "historical Phase 58 marker")

    for rel in [
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/planning/architecture-audit-gap-matrix.md",
        "docs/development/completed-phases-latest.md",
    ]:
        require(errors, rel, ARCHITECTURE_FIRST, "architecture package start")
        require(errors, rel, ARCHITECTURE_LAST, "architecture package end")

    for rel in [
        f"docs/development/{CLOSEOUT_PHASE61}",
        f"docs/development/{CLOSEOUT_PLATFORM}",
        "docs/development/completed-phases/phase-61.md",
    ]:
        if not (ROOT / rel).is_file():
            errors.append(f"closeout/archive file is missing: {rel}")

    for rel in [
        "README.md",
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/planning/phase-map.md",
        "docs/development/current-status.md",
        "docs/development/completed-phases-latest.md",
        "docs/development/completed-phases.md",
    ]:
        require(errors, rel, CLOSEOUT_PHASE61, "Phase 61 closeout link")
        require(errors, rel, CLOSEOUT_PLATFORM, "post-Phase-61 closeout link")

    parity = ROOT / "docs/planning" / PARITY_DOC
    if not parity.is_file():
        errors.append("parity audit document is missing")
    else:
        text = parity.read_text(encoding="utf-8")
        for marker in [
            "VDR Core",
            "Live",
            "epgsearch",
            "RESTfulAPI",
            "VDR-Suite",
            "Backend-scoped Global Search",
            "VDR Remote",
        ]:
            if marker not in text:
                errors.append(f"parity audit document misses marker: {marker}")

    for rel in ["docs/CURRENT.md", "docs/NEW-CHAT-HANDOFF.md", "docs/planning/index.md"]:
        require(errors, rel, GAP_MATRIX, "architecture gap matrix link")

    stale_active_markers = [
        "feature/phase61-metadata-genre-browser",
        "Latest completed slice: Phase 60.15",
        "Next implementation focus: Phase 61",
    ]
    for rel in CANONICAL_STATUS_FILES:
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
    print("Latest completed runtime phase: " + LATEST_PHASE)
    print("Latest hardening block: " + LATEST_HARDENING)
    print("Completed platform features: " + REMOTE_FEATURE + ", " + SEARCH_FEATURE)
    print("Next runtime phase: " + NEXT_RUNTIME)
    return 0


if __name__ == "__main__":
    sys.exit(main())
