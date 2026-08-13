#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PHASE_MAP = ROOT / "docs/planning/phase-map.md"
ROADMAP = ROOT / "docs/planning/roadmap.md"

LATEST = "Phase 63 - Backend Agent and Secure Multi-Site Runtime"
ACTIVE = "Phase 64 - Timer Intent and Multi-Backend Orchestration"
NEXT = "Phase 65 - Streaming Gateway and Media Sessions"
HISTORICAL = "Phase 58 - Frontend and Live Parity"
HARDENING = "Post-Phase 61 Performance Hardening (B1-B4)"

COMPLETED_RANGES = [
    "Phase 1.x-7.x",
    "Phase 8.x",
    "Phase 9.x-29.x",
    "Phase 30.x-44.x",
    "Phase 45.x",
    "Phase 46.x",
    "Phase 47.x-50.50",
    "Phase 51.x-55.6",
    "Phase 56",
    "Phase 57",
    "Phase 58.0-58.90b",
    "Phase 59.00-59.15e",
    "Phase 60.1-60.15",
    "Phase 61",
    "Phase 62",
    "Phase 63",
]

ROADMAP_ORDER = [
    "Phase 64 — Timer Intent and Multi-Backend Orchestration",
    "Phase 65 — Streaming Gateway and Media Sessions",
    "Phase 66 — Legacy OSD Compatibility Bridge",
    "Phase 67 — Public API and Client Compatibility Hardening",
    "Phase 68 — Recommendation and Content Knowledge Graph",
]

REQUIRED_FILES = [
    "docs/architecture/target-platform-architecture.md",
    "docs/planning/domain-dependency-map.md",
    "docs/planning/implementation-dependency-map.md",
    "docs/planning/golden-user-journeys.md",
    "docs/development/phase-61-metadata-genre-performance-closeout.md",
    "docs/development/post-phase-61-platform-runtime-closeout.md",
    "docs/development/completed-phases/phase-61.md",
    "docs/development/phase-62-closeout.md",
    "docs/development/phase-62-slice-2x-runtime-closeout.md",
]


def fail(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    raise SystemExit(1)


def require_markers(text, rel, markers):
    for marker in markers:
        if marker not in text:
            fail(f"{rel} misses required marker: {marker}")


def require_order(text, rel, markers):
    positions = []
    for marker in markers:
        pos = text.find(marker)
        if pos < 0:
            fail(f"{rel} misses ordered marker: {marker}")
        positions.append(pos)
    if positions != sorted(positions):
        fail(f"{rel} does not preserve required phase order")


def main():
    if not PHASE_MAP.is_file() or not ROADMAP.is_file():
        fail("phase-map.md or roadmap.md is missing")

    phase_map = PHASE_MAP.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")

    require_markers(
        phase_map,
        "docs/planning/phase-map.md",
        COMPLETED_RANGES + [LATEST, ACTIVE, NEXT, HARDENING, HISTORICAL],
    )
    require_markers(
        roadmap,
        "docs/planning/roadmap.md",
        [LATEST, ACTIVE, NEXT, HARDENING, HISTORICAL],
    )
    require_order(roadmap, "docs/planning/roadmap.md", ROADMAP_ORDER)

    for rel in REQUIRED_FILES:
        if not (ROOT / rel).is_file():
            fail("required closeout/dependency/planning file is missing: " + rel)

    for rel in [
        "README.md",
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/development/current-status.md",
    ]:
        text = (ROOT / rel).read_text(encoding="utf-8")
        require_markers(text, rel, [LATEST, ACTIVE, NEXT])

    for rel in ["docs/planning/roadmap.md", "docs/planning/phase-map.md"]:
        text = (ROOT / rel).read_text(encoding="utf-8")
        for stale in [
            "Next strict runtime phase:\nPhase 63",
            "Current active runtime slice:\nPhase 63",
            "Current merged main baseline:",
        ]:
            if stale in text:
                fail(f"{rel} still contains stale/volatile planning marker: {stale}")

    print("Phase map coverage check passed.")
    print("Latest completed numbered runtime phase: " + LATEST)
    print("Current active numbered runtime phase: " + ACTIVE)
    print("Next strict numbered runtime phase: " + NEXT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
