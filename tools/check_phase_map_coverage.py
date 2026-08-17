#!/usr/bin/env python3
from pathlib import Path
import re
import sys

from phase_status_contract import ROOT, load_phase_status

PHASE_MAP = ROOT / "docs/planning/phase-map.md"
ROADMAP = ROOT / "docs/planning/roadmap.md"

HISTORICAL = "Phase 58 remains a historical umbrella label only."

BASE_COMPLETED_RANGES = [
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

ROADMAP_PHASE_RE = re.compile(r"^## Phase (\d+) [—-] (.+)$", re.MULTILINE)


def fail(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    raise SystemExit(1)


def require_markers(text, rel, markers):
    for marker in markers:
        if marker not in text:
            fail(f"{rel} misses required marker: {marker}")


def main():
    try:
        status = load_phase_status()
    except ValueError as exc:
        fail(str(exc))

    if not PHASE_MAP.is_file() or not ROADMAP.is_file():
        fail("phase-map.md or roadmap.md is missing")

    phase_map = PHASE_MAP.read_text(encoding="utf-8")
    roadmap = ROADMAP.read_text(encoding="utf-8")

    completed_integer_markers = [
        f"Phase {number}"
        for number in range(61, status.latest_completed_number + 1)
    ]
    require_markers(
        phase_map,
        "docs/planning/phase-map.md",
        BASE_COMPLETED_RANGES
        + completed_integer_markers
        + [status.latest_completed, status.next_phase],
    )
    require_markers(
        roadmap,
        "docs/planning/roadmap.md",
        [status.latest_roadmap_heading, status.next_roadmap_heading],
    )

    # The numbered roadmap headings at and after the latest completed phase must
    # remain strictly increasing. No future phase number is hard-coded here.
    numbered = [(int(number), title) for number, title in ROADMAP_PHASE_RE.findall(roadmap)]
    forward = [(number, title) for number, title in numbered if number >= status.latest_completed_number]
    if not forward:
        fail("roadmap has no numbered forward phase headings")
    numbers = [number for number, _ in forward]
    if numbers != sorted(numbers) or len(numbers) != len(set(numbers)):
        fail("roadmap numbered forward phase headings are not strictly increasing")
    if status.latest_completed_number not in numbers:
        fail("roadmap misses latest completed numbered phase heading")
    if status.next_phase_number not in numbers:
        fail("roadmap misses next strict numbered phase heading")
    latest_index = numbers.index(status.latest_completed_number)
    if latest_index + 1 >= len(numbers) or numbers[latest_index + 1] != status.next_phase_number:
        fail("next strict numbered phase does not immediately follow latest completed phase")

    if HISTORICAL not in phase_map and HISTORICAL not in roadmap:
        fail("historical Phase 58 umbrella marker is missing")

    for rel in REQUIRED_FILES + [status.latest_closeout_rel]:
        if not (ROOT / rel).is_file():
            fail("required closeout/dependency/planning file is missing: " + rel)

    for rel in [
        "docs/CURRENT.md",
        "docs/NEW-CHAT-HANDOFF.md",
        "docs/development/current-status.md",
    ]:
        text = (ROOT / rel).read_text(encoding="utf-8")
        markers = [status.latest_completed, status.next_phase]
        if status.current_active_is_none:
            markers.append(status.next_not_started_marker)
        require_markers(text, rel, markers)

    closeout = status.latest_closeout_path.read_text(encoding="utf-8")
    require_markers(
        closeout,
        status.latest_closeout_rel,
        [f"**Phase {status.latest_completed_number} is completed.**"],
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
    print("Latest completed numbered runtime phase: " + status.latest_completed)
    print("Current active numbered runtime phase: " + status.current_active)
    print("Next strict numbered runtime phase: " + status.next_phase)
    return 0


if __name__ == "__main__":
    sys.exit(main())
