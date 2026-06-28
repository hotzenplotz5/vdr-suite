#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DONE_PHASE = "Phase 55.5n - Roadmap historical coverage alignment"
NEXT_PHASE = "Phase 55.6 - Recording operations audit and safety policy"
PREVIOUS_DONE_PHASE = "Phase 55.5m - Documentation consolidation and roadmap alignment"

TIMELINE_COVERAGE = """## Implementation Timeline Coverage

This section is a milestone coverage map. It is not the detailed chronological phase log.

Detailed phase-by-phase history remains in [Completed Phases](../development/completed-phases.md).

| Phase range | Roadmap coverage |
| --- | --- |
| Phase 1.x-7.x | Core platform, database, repositories, services, REST boundaries and daemon foundation. |
| Phase 8.x | VDR domain objects, adapter boundaries, mock/external backends and RESTfulAPI integration foundation. |
| Phase 9.x-29.x | Multi-backend runtime, backend-aware snapshots, snapshot cache, change feed, live transport and backend-aware API foundation. |
| Phase 30.x-36.x | Recording action validation and guarded execution foundation. |
| Phase 37.x-44.x | Recording action runtime completion, real-backend validation, regression coverage, action safety review and transition toward selective runtime/query hardening. |
| Phase 45.x | Selective EPG search foundation and EPG REST API surface. |
| Phase 46.x | Content classification, metadata domain foundations, person metadata, recording-person search and recording-character search. |
| Phase 47.x-49.x | SearchTimer backend foundation, RESTfulAPI compatibility, real VDR validation, Live/EPGSearch inventory, regex/fuzzy search semantics and native fuzzy capability validation. |
| Phase 50.0-50.50 | SearchTimer user workflow, dry-run/prepare/execute planning, safety gates, real-execution policy, mandatory readback verification and controlled test execution. |
| Phase 51.x | Live plugin parity discovery foundation. |
| Phase 52.x | SearchTimer automation read-only planning, dry-run models, preview endpoint, scheduling plan and safety review. |
| Phase 53.x | SearchTimer title-only REST/workflow preservation and completion audit. |
| Phase 54.x | SearchTimer preview runtime, mutation policy wiring and warm EPG cache architecture. |
| Phase 55.0-55.4e | Feature parity and adapter audit, RESTfulAPI SearchTimer contract fixes, JSON/form-body fixes, discovery provider/runtime wiring and daemon shutdown reset guardrail. |
| Phase 55.5a-55.5m | SearchTimer preview engine contract, native preview capability, real VDR acceptance manifest, acceptance expansion, daemon lifecycle hardening and documentation consolidation. |
| Phase 55.5n | Roadmap historical coverage alignment. |
| Phase 55.6 | Recording operations audit and safety policy. |

---

"""

PLANNED_MAJOR_MILESTONES = """## Planned Major Milestones

### Phase 56 - Library Boundary and Packaging Strategy

Status: Planned.

Goal:
- Separate reusable suite libraries from daemon/application surfaces before the project grows more clients and packaging targets.

Expected outcomes:
- Clear `libs/` versus `apps/` ownership boundary.
- Packaging-oriented build layout without changing the current GNU Make foundation blindly.
- Stable internal API boundaries for daemon, tools and future frontend-facing services.
- Documentation of what remains application glue and what becomes reusable library code.

---

### Phase 57 - Multi-Site Backend Administration and Permissions

Status: Planned.

Goal:
- Support multi-site VDR operation with different permissions per backend, including a read-only secondary-site mode.

Expected outcomes:
- Backend administration API.
- Backend capability visibility and health diagnostics.
- Backend connection validation.
- Backend permission model.
- Read-only secondary backend mode.
- Client-visible backend selection and policy state.
- Preparation for frontend backend administration.

---

### Phase 58 - Frontend and Live Parity for Everyday Usage

Status: Planned.

Goal:
- Close the remaining everyday usability gaps between VDR-Suite and classic VDR frontends while keeping VDR-Suite API-first and multi-backend-ready.

Expected outcomes:
- Practical frontend-ready recording, timer, channel and EPG views.
- Read-only helper surfaces where mutation is not yet safe.
- Improved diagnostics for missing backend features.
- Frontend-ready backend and permission state.
- Web/desktop/mobile client foundations remain downstream of safe API and permission foundations.

---

### Phase 59 - Suite Metadata Database and External Scraper Provider Integration

Status: Planned.

Goal:
- Build a VDR-Suite-owned normalized metadata database while evaluating and using mature external scraper or catalog providers instead of reinventing scraper behavior by default.

Strategic rule:

```text
Use external scrapers and catalog providers when they solve metadata acquisition well.
Normalize useful results into the VDR-Suite metadata database.
Keep provider-specific behavior behind provider boundaries.
```

Expected outcomes:
- Recording metadata capability model.
- Suite-owned normalized metadata database.
- Backend-scoped metadata provider registry.
- Provider evaluation matrix for plugin-backed and external catalog-backed metadata.
- TVScraper provider boundary.
- scraper2vdr provider boundary.
- External catalog provider boundary for movie and TV metadata sources.
- EPG-only fallback provider.
- Normalized recording metadata aggregate.
- Cast, character, director, writer, guest, genre, rating, keyword and external-ID mapping.
- Artwork, poster and backdrop reference model.
- Metadata origin, evidence and confidence model.
- Read-only API for enriched recording metadata.
- Frontend-ready response contracts.

Related planning:
- [Recording Metadata, External Scrapers and Suite Metadata Database Roadmap](tvscraper-recording-metadata-roadmap.md)
- [ADR-0036: TVScraper Recording Metadata Integration Strategy](../adr/ADR-0036-tvscraper-recording-metadata-integration.md)
- [ADR-0037: Suite Metadata Database and External Scraper Strategy](../adr/ADR-0037-suite-metadata-database-and-external-scraper-strategy.md)

---

### Phase 60 - Recommendation and Content Knowledge Graph Foundations

Status: Planned.

Goal:
- Build recommendation and content graph primitives on top of explicit metadata, search history, recording metadata and classification evidence.

Expected outcomes:
- Recommendation candidate model.
- Evidence-based ranking inputs.
- Profile-aware recommendation preparation.
- Explainable recommendation output.
- Relationship-based browsing across recordings, EPG events, people, characters, genres, ratings, backends and user profiles.

---

"""

ROADMAP_DOC = """# Phase 55.5n - Roadmap Historical Coverage Alignment

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Development Index](index.md)
- [Current Project Status](current-status.md)
- [Completed Phases](completed-phases.md)
- [Roadmap](../planning/roadmap.md)
- [Project Status Dashboard](../project-status-dashboard.md)

---

## Purpose

This document records the follow-up documentation step after Phase 55.5m.

Phase 55.5m fixed the high-level project state snapshot. Phase 55.5n fixes the roadmap timeline coverage itself.

The roadmap had visible gaps: it jumped from Phase 30.x-36.x directly to Phase 45.x and did not clearly show the Phase 50-55 tracks as roadmap milestones.

---

## Scope

Phase 55.5n is documentation-only.

It does not add runtime behavior, API behavior, backend mutation, SearchTimer execution or recording action execution.

The scope is:

- add an implementation timeline coverage map to the roadmap
- make Phase 37-44 visible in roadmap history
- make Phase 50-55 visible as roadmap tracks
- remove stale planned Phase 55/56 roadmap labels that conflicted with already completed 55.x work
- keep Phase 55.6 as the next implementation focus

---

## Consolidated State

Latest completed implementation phase:

```text
Phase 55.5n - Roadmap historical coverage alignment
```

Next major implementation milestone:

```text
Phase 55.6 - Recording operations audit and safety policy
```

---

## Required Verification After Consolidation

Run locally:

```bash
make test-docs
make test-phase
```

Recommended guardrails:

```bash
make test-real-vdr-acceptance-manifest
make test-daemon-runtime-shutdown-resets
make test-http-listener-bind-failure-handling
```

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
"""

PHASE_HISTORY_BLOCK = """## Phase 55.5n - Roadmap historical coverage alignment

Status: Completed.

Summary:
- Added a roadmap implementation timeline coverage map so phase ranges between 36 and 45 and between 50 and 55 are visible in the roadmap.
- Made Phase 37.x-44.x visible as the recording action runtime, real-backend validation, regression coverage, safety review and selective runtime/query hardening transition range.
- Made Phase 51.x, 52.x, 53.x, 54.x and 55.0-55.5m visible as explicit roadmap tracks instead of leaving them hidden behind broad SearchTimer and recent-runtime summaries.
- Replaced stale planned Phase 55/56 roadmap entries with later roadmap milestones that no longer conflict with completed 55.x work.
- Kept the phase documentation-only with no runtime behavior, API behavior, backend mutation or recording operation change.

---

"""


def path(relative: str) -> Path:
    return ROOT / relative


def read(relative: str) -> str:
    return path(relative).read_text(encoding="utf-8")


def write(relative: str, text: str) -> None:
    path(relative).write_text(text, encoding="utf-8")


def replace_all_markers(text: str) -> str:
    return text.replace(PREVIOUS_DONE_PHASE, DONE_PHASE)


def replace_or_fail(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit("missing block: " + label)
    return text.replace(old, new, 1)


def insert_after_once(text: str, anchor: str, insertion: str, label: str) -> str:
    if insertion.strip().splitlines()[0] in text:
        return text
    if anchor not in text:
        raise SystemExit("missing anchor: " + label)
    return text.replace(anchor, anchor + insertion, 1)


def update_roadmap() -> None:
    text = read("docs/planning/roadmap.md")
    text = replace_all_markers(text)

    if "## Implementation Timeline Coverage" not in text:
        anchor = "| Vision | The milestone is strategically important but intentionally deferred until earlier foundations exist. |\n\n---\n\n"
        text = insert_after_once(text, anchor, TIMELINE_COVERAGE, "roadmap status terms")

    planned_start = text.index("## Planned Major Milestones")
    long_term_start = text.index("## Long-Term Vision")
    text = text[:planned_start] + PLANNED_MAJOR_MILESTONES + text[long_term_start:]

    text = text.replace("README, roadmap, project progress, current status, project dashboard, development index and completed-phase history now point at the same completed and next milestones.",
                        "README, roadmap, project state, current status, project dashboard, development index and completed-phase history now point at the same completed and next milestones.")

    write("docs/planning/roadmap.md", text)


def update_marker_docs() -> None:
    for relative in [
        "README.md",
        "docs/development/current-status.md",
        "docs/development/index.md",
        "docs/project-status-dashboard.md",
        "docs/development/phase-55.5m-documentation-consolidation.md",
    ]:
        text = read(relative)
        text = replace_all_markers(text)
        write(relative, text)


def update_development_index() -> None:
    text = read("docs/development/index.md")
    entry = "- [Phase 55.5n - Roadmap Historical Coverage Alignment](phase-55.5n-roadmap-historical-coverage.md)"
    if entry not in text:
        anchor = "- [Phase 55.5m - Documentation Consolidation and Roadmap Alignment](phase-55.5m-documentation-consolidation.md)"
        text = insert_after_once(text, anchor, "\n" + entry, "development index 55.5m entry")
    write("docs/development/index.md", text)


def update_completed_phases() -> None:
    text = read("docs/development/completed-phases.md")
    text = replace_all_markers(text)

    if "## Phase 55.5n - Roadmap historical coverage alignment" not in text:
        text = replace_or_fail(text, "## Detailed Phase History\n\n", "## Detailed Phase History\n\n" + PHASE_HISTORY_BLOCK, "completed phase history insertion")

    if "Phase 55.5n - Roadmap historical coverage alignment" not in text.split("## Detailed Phase History", 1)[0]:
        text = text.replace("- Phase 55.5m - Documentation consolidation and roadmap alignment.",
                            "- Phase 55.5m - Documentation consolidation and roadmap alignment.\n- Phase 55.5n - Roadmap historical coverage alignment.", 1)

    text = text.replace("- Roadmap, dashboard and project state alignment.\n- New-chat handoff verification checklist.",
                        "- Roadmap, dashboard and project state alignment.\n- Roadmap historical coverage map.\n- New-chat handoff verification checklist.", 1)

    write("docs/development/completed-phases.md", text)


def write_roadmap_doc() -> None:
    write("docs/development/phase-55.5n-roadmap-historical-coverage.md", ROADMAP_DOC)


def main() -> int:
    update_roadmap()
    update_marker_docs()
    update_development_index()
    update_completed_phases()
    write_roadmap_doc()

    print("Phase 55.5n roadmap historical coverage alignment applied.")
    print("Next checks:")
    print("  make test-docs")
    print("  make test-phase")
    print("  make test-real-vdr-acceptance-manifest")
    print("  make test-daemon-runtime-shutdown-resets")
    print("  make test-http-listener-bind-failure-handling")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
