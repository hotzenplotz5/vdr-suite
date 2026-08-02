#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LATEST_PHASE = "Phase 62 - Identity, RBAC and Accountability Foundation"
HISTORICAL_UMBRELLA = "Phase 58 - Frontend and Live Parity"
LATEST_HARDENING = "Post-Phase 61 Performance Hardening (B1-B4)"
REMOTE_FEATURE = "VDR Remote and Live Overlay hardening (#110)"
SEARCH_FEATURE = "Backend-scoped Global Search (#111)"
REMOTE_ASSET_FEATURE = "Configurable photorealistic VDR Remote (#115)"
NEXT_RUNTIME = "Phase 63 - Backend Agent and Secure Multi-Site Runtime"
ARCHITECTURE_FIRST = "ADR-0042"
ARCHITECTURE_LAST = "ADR-0049"
CLOSEOUT_PHASE61 = "phase-61-metadata-genre-performance-closeout.md"
CLOSEOUT_PLATFORM = "post-phase-61-platform-runtime-closeout.md"
CLOSEOUT_PHASE62 = "phase-62-closeout.md"
CLOSEOUT_SLICE2X = "phase-62-slice-2x-runtime-closeout.md"
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

HISTORY_MARKER_FILES = [
    "README.md",
    "docs/CURRENT.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
    "docs/development/completed-phases-latest.md",
    "docs/development/completed-phases.md",
]

FEATURE_MARKER_FILES = [
    "README.md",
    "docs/planning/roadmap.md",
    "docs/planning/phase-map.md",
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
        require(errors, rel, NEXT_RUNTIME, "next runtime marker")
        require(errors, rel, CLOSEOUT_PHASE62, "Phase 62 closeout link")
        require(errors, rel, CLOSEOUT_SLICE2X, "Slice 2X runtime closeout link")

    for rel in HISTORY_MARKER_FILES:
        require(errors, rel, LATEST_HARDENING, "latest hardening marker")

    for rel in FEATURE_MARKER_FILES:
        require(errors, rel, REMOTE_FEATURE, "completed Remote marker")
        require(errors, rel, SEARCH_FEATURE, "completed Global Search marker")
        require(errors, rel, REMOTE_ASSET_FEATURE, "completed Remote asset marker")

    for rel in [
        "README.md",
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/planning/roadmap.md",
        "docs/planning/phase-map.md",
    ]:
        require(errors, rel, HISTORICAL_UMBRELLA, "historical Phase 58 marker")

    for rel in ["docs/planning/architecture-audit-gap-matrix.md"]:
        require(errors, rel, ARCHITECTURE_FIRST, "architecture package start")
        require(errors, rel, ARCHITECTURE_LAST, "architecture package end")

    required_history_files = [
        f"docs/development/{CLOSEOUT_PHASE61}",
        f"docs/development/{CLOSEOUT_PLATFORM}",
        "docs/development/completed-phases/phase-61.md",
        f"docs/development/{CLOSEOUT_PHASE62}",
        f"docs/development/{CLOSEOUT_SLICE2X}",
    ]
    for rel in required_history_files:
        if not (ROOT / rel).is_file():
            errors.append(f"closeout/archive file is missing: {rel}")

    parity = ROOT / "docs/planning" / PARITY_DOC
    if not parity.is_file():
        errors.append("parity audit document is missing")
    else:
        text = parity.read_text(encoding="utf-8").lower()
        required_markers = [
            "vdr core",
            "live",
            "epgsearch",
            "restfulapi",
            "vdr-suite",
            "global search",
            "remote control",
        ]
        for marker in required_markers:
            if marker not in text:
                errors.append(f"parity audit document misses marker: {marker}")

    for rel in ["docs/CURRENT.md", "docs/NEW-CHAT-HANDOFF.md", "docs/planning/index.md"]:
        require(errors, rel, GAP_MATRIX, "architecture gap matrix link")

    stale_active_markers = [
        "Latest completed slice: Phase 60.15",
        "Next implementation focus: Phase 61",
        "Next runtime implementation focus: Phase 61",
        "Next strict runtime phase:\nPhase 62",
        "Phase 62 state:\nactive and incomplete",
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
    print(
        "Completed platform features: "
        + REMOTE_FEATURE
        + ", "
        + SEARCH_FEATURE
        + ", "
        + REMOTE_ASSET_FEATURE
    )
    print("Next runtime phase: " + NEXT_RUNTIME)
    return 0


if __name__ == "__main__":
    sys.exit(main())
