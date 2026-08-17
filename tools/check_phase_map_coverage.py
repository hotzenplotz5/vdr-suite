#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PHASE_MAP = ROOT / "docs/planning/phase-map.md"
ROADMAP = ROOT / "docs/planning/roadmap.md"

LATEST = "Phase 64 - Timer Intent and Multi-Backend Orchestration"
NEXT = "Phase 65 - Streaming Gateway and Media Sessions"
NOT_STARTED = "Phase 65 has not started"
HISTORICAL = "Phase 58 remains a historical umbrella label only."

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
    "Phase 64",
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
    "docs/development/phase-64-closeout.md",
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
        COMPLETED_RANGES + [LATEST, NEXT],
    )
    require_markers(
        roadmap,
        "docs/planning/roadmap.md",
        [LATEST, NEXT],
    )
    require_order(roadmap, "docs/planning/roadmap.md", ROADMAP_ORDER)

    # The historical Phase-58 umbrella must remain represented without being
    # mistaken for current execution state.
    if HISTORICAL not in phase_map and HISTORICAL not in roadmap:
        fail("historical Phase 58 umbrella marker is missing")

    for rel in REQUIRED_FILES:
        if not (ROOT / rel).is_file():
            fail("required closeout/dependency/planning file is missing: " + rel)

    for rel in [
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/development/current-status.md",
    ]:
        text = (ROOT / rel).read_text(encoding="utf-8")
        require_markers(text, rel, [LATEST, NEXT, NOT_STARTED])

    closeout = (ROOT / "docs/development/phase-64-closeout.md").read_text(
        encoding="utf-8"
    )
    require_markers(
        closeout,
        "docs/development/phase-64-closeout.md",
        [
            "PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS",
            "PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS",
        ],
    )

    readme = (ROOT / "README.md").read_text(encoding="utf-8")
    require_markers(readme, "README.md", ["docs/CURRENT.md", "docs/planning/roadmap.md"])

    for rel in ["docs/planning/roadmap.md", "docs/planning/phase-map.md"]:
        text = (ROOT / rel).read_text(encoding="utf-8")
        for stale in [
            "Next strict runtime phase:\nPhase 63",
            "Current active runtime slice:\nPhase 63",
            "Current merged main baseline:",
            "Latest completed numbered runtime phase:\nPhase 63",
        ]:
            if stale in text:
                fail(f"{rel} still contains stale/volatile planning marker: {stale}")

    print("Phase map coverage check passed.")
    print("Latest completed numbered runtime phase: " + LATEST)
    print("Current active numbered runtime phase: none")
    print("Next strict numbered runtime phase: " + NEXT + " (not started)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
