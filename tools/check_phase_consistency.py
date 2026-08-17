#!/usr/bin/env python3
from pathlib import Path

from phase_status_contract import ROOT, load_phase_status

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
        errors.append(
            f"{path.relative_to(ROOT)} contains forbidden duplicate volatile/stale marker: {marker}"
        )


def roadmap_section(text, heading):
    marker = "## " + heading
    start = text.find(marker)
    if start < 0:
        return ""
    next_heading = text.find("\n## ", start + len(marker))
    return text[start:] if next_heading < 0 else text[start:next_heading]


def main():
    errors = []
    try:
        status_contract = load_phase_status()
    except ValueError as exc:
        print("Phase consistency check failed:")
        print("- " + str(exc))
        return 1

    closeout = status_contract.latest_closeout_path
    required = [CURRENT, README, HANDOFF, STATUS, closeout, ROADMAP, PHASE_MAP] + STABLE_CURRENT_DOCS
    for path in required:
        if not path.is_file():
            errors.append(f"missing status/planning file: {path.relative_to(ROOT)}")

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        return 1

    require(errors, CURRENT, "## Operational status authority")
    require(errors, CURRENT, status_contract.latest_completed)
    require(errors, CURRENT, status_contract.current_active)
    require(errors, CURRENT, status_contract.next_phase)
    require(errors, CURRENT, f"Phase {status_contract.latest_completed_number} Closeout")

    require(
        errors,
        closeout,
        f"**Phase {status_contract.latest_completed_number} is completed.**",
    )

    # README stays stable and delegates live phase/status data to canonical docs.
    require(errors, README, "docs/CURRENT.md")
    require(errors, README, "docs/planning/roadmap.md")

    # Narrative current entry points may mirror the canonical phase tuple, but
    # they must agree with CURRENT.md and must not become commit/PR authorities.
    for path in [HANDOFF, STATUS]:
        require(errors, path, status_contract.latest_completed)
        require(errors, path, status_contract.next_phase)
        if status_contract.current_active_is_none:
            require(errors, path, status_contract.next_not_started_marker)

    for path in [README, HANDOFF, STATUS]:
        for marker in [
            "Current merged main checkpoint:",
            "Current stacked Draft tip:",
            "head checkpoint:",
            "CI checkpoint:",
        ]:
            forbid(errors, path, marker)

    # Canonical current documents must not regress to obsolete operational holds.
    for path in [CURRENT, HANDOFF, STATUS]:
        for marker in [
            "Current stacked implementation checkpoint:",
            "planning hold after the PR-#190 checkpoint",
            "No Phase-64 successor implementation is currently authorized",
            "no `#191` are currently authorized",
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

    roadmap = read(ROADMAP)
    completed_section = roadmap_section(roadmap, status_contract.latest_roadmap_heading)
    next_section = roadmap_section(roadmap, status_contract.next_roadmap_heading)
    if "Status: **Completed.**" not in completed_section:
        errors.append(
            "docs/planning/roadmap.md latest completed phase section lacks Completed status: "
            + status_contract.latest_roadmap_heading
        )
    if status_contract.current_active_is_none:
        if "Status: **Next; not started.**" not in next_section:
            errors.append(
                "docs/planning/roadmap.md next phase section lacks Next; not started status: "
                + status_contract.next_roadmap_heading
            )

    if errors:
        print("Phase consistency check failed:")
        for error in errors:
            print("- " + error)
        return 1

    print("Phase consistency check passed.")
    print("Volatile status authority: docs/CURRENT.md")
    print("Latest completed numbered runtime phase: " + status_contract.latest_completed)
    print("Current active numbered runtime phase: " + status_contract.current_active)
    print("Next strict numbered runtime phase: " + status_contract.next_phase)
    print("Latest closeout: " + status_contract.latest_closeout_rel)
    print("Stable current/architecture docs: no duplicate volatile repository snapshot")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
