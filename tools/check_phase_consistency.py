#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CURRENT = ROOT / "docs" / "CURRENT.md"
README = ROOT / "README.md"
HANDOFF = ROOT / "docs" / "NEW-CHAT-HANDOFF.md"
STATUS = ROOT / "docs" / "development" / "current-status.md"
ROADMAP = ROOT / "docs" / "planning" / "roadmap.md"
PHASE_MAP = ROOT / "docs" / "planning" / "phase-map.md"

STABLE_CURRENT_DOCS = [
    ROOT / "docs" / "index.md",
    ROOT / "docs" / "project-overview.md",
    ROOT / "docs" / "project-status-dashboard.md",
    ROOT / "docs" / "planning" / "index.md",
    ROOT / "docs" / "development" / "index.md",
    ROOT / "docs" / "architecture" / "index.md",
    ROOT / "docs" / "architecture" / "target-platform-architecture.md",
    ROOT / "docs" / "development" / "current-architecture-state.md",
    ROOT / "docs" / "planning" / "architecture-audit-gap-matrix.md",
    ROOT / "docs" / "planning" / "implementation-dependency-map.md",
    ROOT / "docs" / "planning" / "parity-audit-and-frontend-gap-roadmap.md",
]


def read(path):
    return path.read_text(encoding="utf-8")


def require(errors, path, marker):
    if marker not in read(path):
        errors.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


def forbid(errors, path, marker):
    if marker in read(path):
        errors.append(f"{path.relative_to(ROOT)} contains forbidden duplicate volatile/stale marker: {marker}")


def main():
    errors = []

    required = [CURRENT, README, HANDOFF, STATUS, ROADMAP, PHASE_MAP] + STABLE_CURRENT_DOCS
    for path in required:
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
    require(errors, README, "broad")
    require(errors, README, "Timer UI")
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

    # Stable current/navigation/architecture documents must point to CURRENT
    # rather than maintaining a second volatile repository snapshot.
    for path in STABLE_CURRENT_DOCS:
        text = read(path)
        if "CURRENT.md" not in text and "Current State" not in text:
            errors.append(
                f"{path.relative_to(ROOT)} must delegate volatile state to docs/CURRENT.md"
            )

        for marker in [
            "Latest completed numbered runtime phase:\nPhase 61",
            "Latest completed numbered runtime phase:\nPhase 62",
            "Next strict runtime phase:\nPhase 62",
            "Next strict runtime phase:\nPhase 63",
            "none; Phase 63 is planned but not started",
            "Phase 63 is not complete.",
            "Active contract work in Draft PR #138",
            "Status: active through bounded Slice 1 in Draft PR #137",
            "Baseline reconciled on 2026-07-27",
            "Current merged main baseline:",
            "Current verified position",
            "## Current markers",
        ]:
            forbid(errors, path, marker)

    # The target architecture must remain architecture, not an operational log.
    target = ROOT / "docs" / "architecture" / "target-platform-architecture.md"
    require(errors, target, "Safe mutation and durable execution target")
    require(errors, target, "fenced Agent/native command")
    require(errors, target, "authoritative readback and verification")
    for marker in [
        "Implemented on merged `main @",
        "Active contract work in Draft PR",
        "Current implementation overlay",
    ]:
        forbid(errors, target, marker)

    # Planning dependency documents describe order, not active authorization.
    implementation_map = ROOT / "docs" / "planning" / "implementation-dependency-map.md"
    require(errors, implementation_map, "This map answers **what must precede what**")
    forbid(errors, implementation_map, "Status: active through bounded Slice 1 in Draft PR #137")

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
    print("Stable current/architecture docs: no duplicate volatile phase snapshot")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
