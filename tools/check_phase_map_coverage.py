#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
PREVIOUS = "Phase 55.5n - Roadmap historical coverage alignment"
LATEST = "Phase 55.5o - Phase map and roadmap simplification"
NEXT = "Phase 55.6 - Recording operations audit and safety policy"

COMPLETED_ROWS = [
    ("Phase 1.x-7.x", "Completed", "Core Platform", "Database, repositories, services, REST boundaries and daemon foundation."),
    ("Phase 8.x", "Completed", "VDR Backend", "VDR domain objects, adapter boundaries and RESTfulAPI integration foundation."),
    ("Phase 9.x-29.x", "Completed", "Multi-Backend Runtime", "Backend registry, snapshots, change feed and live transport foundation."),
    ("Phase 30.x-36.x", "Completed", "Recording Actions", "Recording action validation and guarded execution foundation."),
    ("Phase 37.x-44.x", "Completed", "Recording Runtime Hardening", "Recording action runtime completion, real-backend validation, regression coverage and safety transition."),
    ("Phase 45.x", "Completed", "EPG Search", "Selective EPG query/search foundation and EPG REST API surface."),
    ("Phase 46.x", "Completed", "Metadata and People", "Content classification, metadata domain foundations, person metadata, recording-person search and character search."),
    ("Phase 47.x-49.x", "Completed", "SearchTimer Backend", "SearchTimer backend foundation, RESTfulAPI compatibility, real VDR validation and native capability validation."),
    ("Phase 50.0-50.50", "Completed", "SearchTimer Workflow", "User workflow, dry-run and execution planning, safety gates, readback verification and controlled execution."),
    ("Phase 51.x", "Completed", "Live Parity Discovery", "Live plugin parity discovery and gap visibility foundation."),
    ("Phase 52.x", "Completed", "SearchTimer Automation Planning", "Read-only planning, dry-run models, preview endpoint, daemon scheduling plan and automation safety review."),
    ("Phase 53.x", "Completed", "SearchTimer Completion Audit", "Title-only REST/workflow preservation and completion audit."),
    ("Phase 54.x", "Completed", "SearchTimer Preview Runtime", "Preview runtime, mutation policy wiring and warm EPG cache architecture."),
    ("Phase 55.0-55.4e", "Completed", "Adapter and Runtime Hardening", "Feature parity and adapter audit, RESTfulAPI contract fixes, discovery wiring and daemon shutdown reset guardrail."),
    ("Phase 55.5a-55.5n", "Completed", "Acceptance and Documentation", "Preview engine contract, native preview capability, real VDR acceptance, lifecycle hardening and roadmap historical coverage."),
    ("Phase 55.5o", "Completed", "Phase Map and Roadmap", "Canonical phase map, simplified roadmap and phase-map coverage guardrail."),
]

PLANNED_ROWS = [
    ("Phase 55.6", "Next", "Recording Operations Audit", "Audit live recording operations and define safety policy before destructive real-backend probes expand."),
    ("Phase 56", "Planned", "Library Boundary and Packaging", "Separate reusable libraries from daemon/application surfaces."),
    ("Phase 57", "Planned", "Multi-Site Backend Administration and Permissions", "Support multi-site operation with backend-specific permissions and read-only secondary-site mode."),
    ("Phase 58", "Planned", "Frontend and Live Parity", "Build frontend-ready everyday recording, timer, channel and EPG views after safe API and permission foundations."),
    ("Phase 59", "Planned", "Suite Metadata Database and External Providers", "Build a suite-owned metadata database while using mature external scraper/catalog providers behind boundaries."),
    ("Phase 60", "Vision", "Recommendation and Content Knowledge Graph", "Build recommendation and graph primitives after metadata and frontend foundations mature."),
]

REQUIRED_RANGES = [row[0] for row in COMPLETED_ROWS] + [row[0] for row in PLANNED_ROWS]
STALE_ROADMAP_MARKERS = [
    "### Recording Action Foundation",
    "### EPG Search Foundation",
    "### Phase 55 - Backend Management",
    "### Phase 56 - Backend Capability Matrix",
]


def p(rel):
    return ROOT / rel


def read(rel):
    return p(rel).read_text(encoding="utf-8")


def write(rel, text):
    p(rel).write_text(text, encoding="utf-8")


def table(rows, last_header="Result"):
    out = ["| Range | Status | Track | " + last_header + " |", "| --- | --- | --- | --- |"]
    for row in rows:
        out.append("| " + " | ".join(row) + " |")
    return "\n".join(out)


def phase_map_doc():
    return """# VDR-Suite Phase Map

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)

---

## Purpose

This file is the canonical compact phase map for VDR-Suite.

It exists so new chats, documentation updates and roadmap changes can quickly see what is completed, what is current and what is planned without reconstructing state from the full chronological phase history.

Detailed phase-by-phase history remains in [Completed Phases](../development/completed-phases.md).

---

## Completed Phase Ranges

""" + table(COMPLETED_ROWS) + """

---

## Current and Planned Phase Ranges

""" + table(PLANNED_ROWS, "Goal") + """

---

## Maintenance Rules

- This file is the compact source of truth for phase-range coverage.
- The roadmap should summarize this file instead of duplicating completed milestone details.
- Planned phase numbers must not conflict with completed phase ranges.
- When a new completed phase range appears, update this file and run the phase-map coverage check.

Verification:

```bash
make test-phase-map-coverage
make test-docs
make test-phase
```

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Planning Index](index.md)
"""


def roadmap_doc():
    completed_summary = "\n".join("- " + r[0] + ": " + r[2] + "." for r in COMPLETED_ROWS)
    return """# VDR-Suite Roadmap

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Project Overview](../project-overview.md)
- [Planning Index](index.md)
- [Phase Map](phase-map.md)
- [Current Project Status](../development/current-status.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Completed Phases](../development/completed-phases.md)
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)

---

## Current Position

```text
Completed implementation state
Phase 55.5o - Phase map and roadmap simplification

Current implementation focus
Phase 55.6 - Recording operations audit and safety policy
```

---

## Purpose

This roadmap describes the current direction of VDR-Suite without duplicating the full phase history.

The compact source of truth for phase-range coverage is [Phase Map](phase-map.md).

Detailed chronological implementation history belongs to [Completed Phases](../development/completed-phases.md).

---

## Phase Map Summary

See [Phase Map](phase-map.md) for the canonical compact table.

Completed foundation ranges:

""" + completed_summary + """

---

## Current Milestone

### Phase 55.6 - Recording Operations Audit and Safety Policy

Status: Next.

Goal:
- Audit live recording operations before enabling or expanding real write-capable paths.

Planned outcomes:
- Confirm which recording operations remain read-only, dry-run-only or destructive.
- Define operator confirmation, permission, backend allowlist and safety-policy requirements.
- Keep destructive real VDR probes explicitly opt-in.
- Preserve the current real VDR acceptance suite as safe/dry-run by default.
- Prepare later recording operations implementation without silently opening mutation paths.

---

## Planned Major Milestones

""" + planned_sections() + """

## Roadmap Maintenance Rules

- [Phase Map](phase-map.md) is the compact source of truth for phase-range coverage.
- This roadmap describes direction and should not duplicate the detailed completed phase log.
- Detailed chronological implementation history belongs in [Completed Phases](../development/completed-phases.md).
- Project status snapshots belong in [Current Project Status](../development/current-status.md) and [Project Status Dashboard](../project-status-dashboard.md).
- Planned phase numbers must not conflict with completed phase ranges.

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Project Overview](../project-overview.md)
"""


def planned_sections():
    sections = []
    for phase, status, track, goal in PLANNED_ROWS[1:]:
        sections.append("### " + phase + " - " + track + "\n\nStatus: " + status + ".\n\nGoal:\n- " + goal + "\n\n---\n")
    return "\n".join(sections)


def phase_doc():
    return """# Phase 55.5o - Phase Map and Roadmap Simplification

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Phase Map](../planning/phase-map.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)

---

## Purpose

This document records the documentation simplification step after Phase 55.5n.

Phase 55.5n made missing roadmap ranges visible, but the roadmap still duplicated too much historical milestone text.

Phase 55.5o introduces a compact canonical phase map and simplifies the roadmap so future chats and documentation updates have one clear entry point for project state.

---

## Scope

Phase 55.5o is documentation and guardrail only.

It does not add runtime behavior, API behavior, backend mutation, SearchTimer execution or recording action execution.

---

## Consolidated State

Latest completed implementation phase:

```text
Phase 55.5o - Phase map and roadmap simplification
```

Next major implementation milestone:

```text
Phase 55.6 - Recording operations audit and safety policy
```

---

## Required Verification

```bash
make test-docs
make test-phase
make test-phase-map-coverage
make test-real-vdr-acceptance-manifest
make test-daemon-runtime-shutdown-resets
make test-http-listener-bind-failure-handling
```

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
"""


def replace_markers():
    for rel in [
        "README.md",
        "docs/development/current-status.md",
        "docs/development/index.md",
        "docs/project-status-dashboard.md",
        "docs/development/completed-phases.md",
        "docs/development/phase-55.5n-roadmap-historical-coverage.md",
    ]:
        write(rel, read(rel).replace(PREVIOUS, LATEST))


def update_planning_index():
    text = read("docs/planning/index.md").replace(PREVIOUS, LATEST)
    text = text.replace("- [Project Progress](project-progress.md)", "- [Phase Map](phase-map.md)\n- [Project State Snapshot](project-progress.md)")
    text = text.replace("Phase 54.3 - SearchTimer warm EPG cache implementation", LATEST)
    text = text.replace("Lazy Recording Loading and Backend-Scoped Recording Refresh", NEXT)
    text = text.replace("- [Roadmap](roadmap.md) contains long-term direction.\n- [Project Progress](project-progress.md) contains high-level progress percentages and the active milestone.",
                        "- [Phase Map](phase-map.md) contains compact phase-range coverage and is the first place to update when completed/planned ranges change.\n- [Roadmap](roadmap.md) contains long-term direction and should not duplicate the full completed phase history.\n- [Project State Snapshot](project-progress.md) contains the generated high-level state snapshot used by README and status documents.")
    write("docs/planning/index.md", text)


def update_development_index():
    text = read("docs/development/index.md")
    entry = "- [Phase 55.5o - Phase Map and Roadmap Simplification](phase-55.5o-phase-map-and-roadmap-simplification.md)"
    anchor = "- [Phase 55.5n - Roadmap Historical Coverage Alignment](phase-55.5n-roadmap-historical-coverage.md)"
    if entry not in text:
        text = text.replace(anchor, anchor + "\n" + entry, 1)
    write("docs/development/index.md", text)


def update_completed_phases():
    text = read("docs/development/completed-phases.md")
    if "## Phase 55.5o - Phase map and roadmap simplification" not in text:
        block = "## Phase 55.5o - Phase map and roadmap simplification\n\nStatus: Completed.\n\nSummary:\n- Added `docs/planning/phase-map.md` as the compact canonical phase-range overview.\n- Simplified `docs/planning/roadmap.md` so it references the phase map instead of duplicating historical completed milestone details.\n- Added `tools/check_phase_map_coverage.py` as a guardrail against missing phase ranges and stale planned blocks.\n- Kept Phase 55.6 as the next implementation focus.\n- Kept this phase documentation/guardrail-only with no runtime behavior, API behavior or backend mutation change.\n\n---\n\n"
        text = text.replace("## Detailed Phase History\n\n", "## Detailed Phase History\n\n" + block, 1)
    if "- Phase 55.5o - Phase map and roadmap simplification." not in text:
        text = text.replace("- Phase 55.5n - Roadmap historical coverage alignment.", "- Phase 55.5n - Roadmap historical coverage alignment.\n- Phase 55.5o - Phase map and roadmap simplification.", 1)
    write("docs/development/completed-phases.md", text)


def update_makefile_group():
    text = read("mk/local-test-groups.mk")
    text = text.replace("test-real-vdr-acceptance-manifest", "test-real-vdr-acceptance-manifest test-phase-map-coverage", 1)
    text = text.replace("\ttest-real-vdr-acceptance-manifest", "\ttest-real-vdr-acceptance-manifest \\\n\ttest-phase-map-coverage", 1)
    if "test-phase-map-coverage:" not in text:
        text = text.replace("test-real-vdr-acceptance-manifest:\n\tpython3 tools/real-vdr-acceptance/runner.py --validate-only\n",
                            "test-real-vdr-acceptance-manifest:\n\tpython3 tools/real-vdr-acceptance/runner.py --validate-only\n\n\ntest-phase-map-coverage:\n\tpython3 tools/check_phase_map_coverage.py\n", 1)
    write("mk/local-test-groups.mk", text)


def apply_changes():
    write("docs/planning/phase-map.md", phase_map_doc())
    write("docs/planning/roadmap.md", roadmap_doc())
    write("docs/development/phase-55.5o-phase-map-and-roadmap-simplification.md", phase_doc())
    replace_markers()
    update_planning_index()
    update_development_index()
    update_completed_phases()
    update_makefile_group()
    print("Phase 55.5o phase map and roadmap simplification applied.")


def error(message):
    print("Phase map coverage check failed:")
    print("- " + message)
    sys.exit(1)


def check():
    phase_map = read("docs/planning/phase-map.md")
    roadmap = read("docs/planning/roadmap.md")
    if "# VDR-Suite Phase Map" not in phase_map:
        error("phase-map.md does not contain the expected title")
    for item in REQUIRED_RANGES:
        if item not in phase_map:
            error("phase-map.md misses required range: " + item)
    if "[Phase Map](phase-map.md)" not in roadmap:
        error("roadmap.md does not link to phase-map.md")
    for item in STALE_ROADMAP_MARKERS:
        if item in roadmap:
            error("roadmap.md still contains stale block: " + item)
    for rel in ["README.md", "docs/development/current-status.md", "docs/development/index.md", "docs/project-status-dashboard.md", "docs/development/completed-phases.md", "docs/planning/roadmap.md"]:
        if LATEST not in read(rel):
            error(rel + " misses latest completed phase marker")
    for rel in ["README.md", "docs/development/current-status.md", "docs/development/index.md", "docs/project-status-dashboard.md", "docs/planning/roadmap.md"]:
        if NEXT not in read(rel):
            error(rel + " misses next implementation focus marker")
    print("Phase map coverage check passed.")


def main():
    if len(sys.argv) == 2 and sys.argv[1] == "--apply":
        apply_changes()
        return 0
    check()
    return 0


if __name__ == "__main__":
    sys.exit(main())
