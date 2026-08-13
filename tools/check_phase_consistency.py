#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CURRENT = ROOT / "docs" / "CURRENT.md"
README = ROOT / "README.md"
HANDOFF = ROOT / "docs" / "NEW-CHAT-HANDOFF.md"
STATUS = ROOT / "docs" / "development" / "current-status.md"
ROADMAP = ROOT / "docs" / "planning" / "roadmap.md"
PHASE_MAP = ROOT / "docs" / "planning" / "phase-map.md"


def read(path):
    return path.read_text(encoding="utf-8")


def require(errors, path, marker):
    if marker not in read(path):
        errors.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


def forbid(errors, path, marker):
    if marker in read(path):
        errors.append(f"{path.relative_to(ROOT)} contains forbidden duplicate volatile marker: {marker}")


def main():
    errors = []

    for path in [CURRENT, README, HANDOFF, STATUS, ROADMAP, PHASE_MAP]:
        if not path.is_file():
            errors.append(f"missing status/planning file: {path.relative_to(ROOT)}")

    if errors:
        for error in errors:
            print("- " + error)
        return 1

    require(errors, CURRENT, "## Operational status authority")
    require(errors, CURRENT, "Phase 63 - Backend Agent and Secure Multi-Site Runtime")
    require(errors, CURRENT, "Phase 64 - Timer Intent and Multi-Backend Orchestration")
    require(errors, CURRENT, "Phase 65 - Streaming Gateway and Media Sessions")
    require(errors, CURRENT, "PR #190 - Add disabled SuiteBridge Timer delete transport")
    require(errors, CURRENT, "No Phase-64 successor implementation is currently authorized")

    # README stays stable and delegates live phase/status data to canonical docs.
    require(errors, README, "docs/CURRENT.md")
    require(errors, README, "docs/planning/roadmap.md")
    require(errors, README, "broad Timer UI")
    require(errors, README, "Streaming")

    for path in [HANDOFF, STATUS]:
        require(errors, path, "Phase 64")
        require(errors, path, "Phase 65")
        require(errors, path, "broad")
        require(errors, path, "Timer UI")
        require(errors, path, "Streaming")

    for path in [README, HANDOFF, STATUS]:
        for marker in [
            "Current merged main checkpoint:",
            "Current stacked Draft tip:",
            "head checkpoint:",
            "CI checkpoint:",
        ]:
            forbid(errors, path, marker)

    for path in [CURRENT, HANDOFF, STATUS]:
        require(errors, path, "broad")
        require(errors, path, "Timer UI")
        require(errors, path, "Streaming")

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Phase consistency check passed.")
    print("Volatile status authority: docs/CURRENT.md")
    print("Latest completed numbered runtime phase: Phase 63")
    print("Current active numbered runtime phase: Phase 64")
    print("Next strict numbered runtime phase after Phase 64: Phase 65")
    print("Implementation hold: after PR #190; no successor currently authorized")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
