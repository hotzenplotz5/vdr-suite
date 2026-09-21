#!/usr/bin/env python3
from __future__ import annotations

import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DONE_PHASE = "Phase 55.5m - Documentation consolidation and roadmap alignment"
NEXT_PHASE = "Phase 55.6 - Recording operations audit and safety policy"
FOLLOW_UP = "Recording operations audit and safety policy"
CURRENT_FOUNDATION = "Recording Operations Audit and Safety Policy"

PROJECT_STATE_SOURCE = """# VDR-Suite Project State Snapshot

## Navigation

- [README](../../README.md)
- [Documentation Index](../index.md)
- [Planning Index](index.md)
- [Roadmap](roadmap.md)
- [Lazy Recording Loading](lazy-recording-loading.md)
- [Project Status Dashboard](../project-status-dashboard.md)
- [Current Project Status](../development/current-status.md)

---

## Purpose

This file is the source for the high-level project state block copied into README, Current Status and Project Status Dashboard.

It is intentionally **not** a percentage-based product progress tracker.

VDR-Suite has many completed foundations, but it is not production-complete. Some domains are deliberately read-only, dry-run-only or guarded until later safety phases.

The generated state blocks are written by:

    tools/update_project_progress.py

---

<!-- PROJECT_STATE_SNAPSHOT_START -->
## Project State Snapshot

This is a verified implementation-state snapshot, not a product-completion percentage.

### Verified foundations

- Core runtime and daemon foundation
- VDR backend adapter and RESTfulAPI integration foundation
- Backend registry and multi-backend runtime foundation
- Snapshot cache, snapshot access and change-feed foundation
- REST routing and JSON response boundaries
- Recording query foundation
- Recording action validation and guarded execution foundation
- Selective EPG query and EPG search foundation
- Content classification and person metadata foundations
- Recording person and character search foundations
- SearchTimer backend, validation, planning and workflow foundations
- SearchTimer safety gates, readback verification and production mutation policy foundations
- Live parity discovery foundation
- Real VDR acceptance manifest and runner foundation
- Daemon lifecycle hardening for duplicate bind failures and SIGTERM shutdown

### Verified real-runtime evidence

- Real VDR acceptance currently passes 20/20 safe and dry-run probes.
- Duplicate daemon start on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- GitHub Actions verification is required before runtime-related phases are considered complete.

### Guarded or deliberately incomplete areas

- SearchTimer production mutation remains gated and closed by default.
- Recording operations need a dedicated safety audit before destructive real-backend probes are expanded.
- Lazy recording loading is still a required follow-up for large real recording catalogs and multi-backend scaling.
- Suite-owned metadata database and external scraper/provider strategy are planned but not yet implemented as the final metadata product.
- Authentication, authorization, per-backend permissions and read-only secondary-site policy are still planned.
- Web, Windows, Android, iOS and TV frontends are planned product layers, not completed foundations.

### Current active focus

```text
Phase 55.6 - Recording operations audit and safety policy
```

### Later strategic milestones

- Library boundary and packaging strategy
- Multi-site backend federation and permissions
- Frontend foundation
- Suite metadata database and external provider integration
- Safe production-grade recording operations

Progress source: {{PROJECT_PROGRESS_SOURCE_LINK}}
<!-- PROJECT_STATE_SNAPSHOT_END -->

---

## Maintenance Rule

Update this file when the verified project state changes.

After editing this file, run:

    python3 tools/update_project_progress.py
    make test-docs
    make test-phase

---

## Back

- [Back to README](../../README.md)
- [Back to Documentation Index](../index.md)
- [Back to Planning Index](index.md)
"""

CONSOLIDATION_DOC = """# Phase 55.5m - Documentation Consolidation and Roadmap Alignment

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

This document records the documentation consolidation step after the real VDR acceptance and daemon lifecycle hardening work.

The implementation and runtime work had advanced beyond the high-level project documentation. Several status files still described Phase 54.3e / Phase 54.3 as the current position even though the repository had already verified the Phase 55.5 real VDR acceptance and runtime lifecycle hardening work.

---

## Scope

Phase 55.5m is a documentation-only consolidation phase.

It does not add runtime behavior, API behavior, backend mutation, SearchTimer execution or recording action execution.

The scope is:

- align latest completed implementation markers across tracked documentation files
- align next implementation focus markers across tracked documentation files
- replace misleading percentage-style progress with a verified project state snapshot
- add recent Phase 55.5 real VDR acceptance and runtime lifecycle work to completed-phase history
- keep the new-chat handoff and required verification checklist visible
- preserve the rule that runtime/API phases need local evidence plus GitHub Actions evidence before completion

---

## Consolidated State

Latest completed implementation phase:

```text
Phase 55.5m - Documentation consolidation and roadmap alignment
```

Next major implementation milestone:

```text
Phase 55.6 - Recording operations audit and safety policy
```

---

## Verified Prior Runtime Evidence

The documentation consolidation is based on already verified local and GitHub evidence:

```text
Real VDR acceptance: 20/20 probes passed
Duplicate daemon start on occupied port: clean exit=1, no abort, no core dump
SIGTERM daemon shutdown: clean stop, no kill -9 required, port 18080 released
GitHub Actions for runtime hardening: docs-check success, fast-regression-test success, daemon build success
```

---

## Required Verification After Consolidation

Run locally:

```bash
make test-docs
make test-phase
make test-real-vdr-acceptance-manifest
make test-daemon-runtime-shutdown-resets
make test-http-listener-bind-failure-handling
```

Then verify GitHub Actions:

```text
docs-check: success
fast-regression-test: success
Build daemon: success
```

---

## Back

- [Development Index](index.md)
- [Documentation Index](../index.md)
"""

COMPLETED_OVERVIEW_BLOCK = """### Real VDR Acceptance and Runtime Lifecycle Foundation

Status: Completed.

Purpose:
- Establish repeatable real VDR acceptance checks for safe and dry-run API coverage.
- Make daemon lifecycle behavior operator-safe for duplicate starts and normal SIGTERM shutdown.
- Preserve a new-chat handoff checklist so local runtime, acceptance, documentation and GitHub Actions checks are not forgotten.

Completed phases:
- Phase 55.5c - Real VDR acceptance manifest foundation.
- Phase 55.5d - Acceptance manifest route validation.
- Phase 55.5e - Acceptance JSON report output.
- Phase 55.5g - Expected JSON values in acceptance runner.
- Phase 55.5h - Acceptance manifest safe-read expansion.
- Phase 55.5i - SearchTimer plan dry-run acceptance probe.
- Phase 55.5j - Safe query acceptance coverage.
- Phase 55.5k - Real VDR acceptance cold-recording timeout stabilization.
- Phase 55.5l - Daemon startup/shutdown hardening.
- Phase 55.5m - Documentation consolidation and roadmap alignment.

Key outcomes:
- Real VDR acceptance currently verifies 20/20 safe and dry-run probes.
- Duplicate daemon startup on an occupied HTTP port exits cleanly with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9` and releases port 18080.
- New-chat handoff documentation now records the required local, real VDR and GitHub Actions evidence before declaring runtime phases complete.

---

"""

DETAILED_PHASE_BLOCK = """## Phase 55.5m - Documentation consolidation and roadmap alignment

Status: Completed.

Summary:
- Consolidated README, roadmap, project state snapshot, current status, project dashboard, development index and completed-phase history after the Phase 55.5 real VDR acceptance and daemon lifecycle hardening work.
- Replaced misleading percentage-style progress with a verified project state snapshot that separates verified foundations from guarded, deliberately incomplete and planned work.
- Moved the tracked latest completed implementation marker to Phase 55.5m across the documentation files monitored by `check_phase_consistency.py`.
- Moved the tracked next implementation focus to Phase 55.6 recording operations audit and safety policy.
- Preserved the new-chat handoff checklist for local runtime, real VDR acceptance, documentation and GitHub Actions verification.
- Kept this phase documentation-only with no runtime behavior, API behavior, backend mutation or recording operation change.

---

## Phase 55.5l - Daemon startup/shutdown hardening

Status: Completed.

Summary:
- Changed HTTP listener bind failure handling so a duplicate daemon start on an occupied port exits cleanly with status 1 instead of throwing an uncaught exception and producing a core dump.
- Added guardrail coverage for bind failure behavior.
- Changed the HTTP listener loop to poll for stop requests and break cleanly on signal interruption.
- Verified against a real local daemon that SIGTERM stops the process without `kill -9` and releases port 18080.
- Verified the final real VDR acceptance run after hardening with 20/20 probes passing.

---

## Phase 55.5k - Real VDR acceptance cold-recording timeout stabilization

Status: Completed.

Summary:
- Added per-probe timeout support to the real VDR acceptance runner.
- Increased the cold recording query acceptance timeout so large real recording catalogs do not cascade into unrelated probe timeouts.
- Preserved the default timeout for ordinary probes.
- Added timeoutSeconds to the JSON report output for each probe result.
- Verified that the cold real VDR acceptance run passed 20/20 probes.

---

## Phase 55.5j - Safe query acceptance coverage

Status: Completed.

Summary:
- Extended the real VDR acceptance manifest with safe query probes for recording query, EPG search, EPG now/next and EPG time-window endpoints.
- Kept all new probes safe and non-mutating.
- Exposed the practical runtime cost of large recording catalogs during cold acceptance runs.

---

## Phase 55.5i - SearchTimer plan dry-run acceptance probe

Status: Completed.

Summary:
- Added a dry-run SearchTimer workflow planning acceptance probe.
- Verified that create planning returns a valid dry-run plan with create/readback steps and explicit operator confirmation requirements.
- Kept backend mutation out of acceptance scope.

---

## Phase 55.5h - Acceptance manifest safe-read expansion

Status: Completed.

Summary:
- Expanded the real VDR acceptance manifest with safe read probes for channels, timers, recordings, events and SearchTimer lists.
- Kept the acceptance manifest focused on safe and dry-run coverage.

---

## Phase 55.5g - Expected JSON values in acceptance runner

Status: Completed.

Summary:
- Added expectedJsonValues support to the real VDR acceptance runner.
- Strengthened SearchTimer workflow validation acceptance from HTTP-200-only to response-value verification.
- Verified validation semantics for operation, execution mode, workflow steps and safety flags.

---

## Phase 55.5e - Acceptance JSON report output

Status: Completed.

Summary:
- Added optional JSON report output to the real VDR acceptance runner.
- Captured manifest name, base URL, risk level, totals, duration and per-probe results.

---

## Phase 55.5d - Acceptance manifest route validation

Status: Completed.

Summary:
- Added validation that acceptance manifest routes are present in ApiRouter.
- Prevented stale acceptance entries from silently drifting away from routed API paths.

---

## Phase 55.5c - Real VDR acceptance manifest foundation

Status: Completed.

Summary:
- Added the real VDR acceptance manifest and runner foundation.
- Added initial safe and dry-run probes for dashboard, VDR overview, status, health, backend registry, capabilities, runtime summary and SearchTimer workflow paths.
- Wired manifest validation into the local CI-fast/VDR test groups.

---

## Phase 55.4e - Daemon runtime shutdown reset guardrail

Status: Completed.

Summary:
- Completed daemon runtime shutdown reset coverage for recently added runtime components.
- Added a guardrail that checks shutdown reset coverage for runtime-owned services/controllers.
- Wired the shutdown reset guardrail into the local test groups.

---

"""


def path(relative: str) -> Path:
    return ROOT / relative


def read(relative: str) -> str:
    return path(relative).read_text(encoding="utf-8")


def write(relative: str, text: str) -> None:
    path(relative).write_text(text, encoding="utf-8")


def replace_indented_marker(text: str, marker: str, value: str) -> str:
    pattern = re.compile(r"(" + re.escape(marker) + r"\n\n)    .*")
    updated, count = pattern.subn(r"\1    " + value, text, count=1)
    if count != 1:
        raise SystemExit("marker not found or not unique: " + marker)
    return updated


def replace_fenced_marker(text: str, marker: str, value: str) -> str:
    pattern = re.compile(r"(" + re.escape(marker) + r"\n\n```text\n)(.*?)(\n```)", re.S)
    updated, count = pattern.subn(r"\1" + value + r"\3", text, count=1)
    if count != 1:
        raise SystemExit("fenced marker not found or not unique: " + marker)
    return updated


def ensure_item_after(text: str, anchor: str, item: str) -> str:
    if item in text:
        return text
    if anchor not in text:
        raise SystemExit("anchor not found: " + anchor)
    return text.replace(anchor, anchor + "\n" + item, 1)


def update_project_state_source() -> None:
    write("docs/planning/project-progress.md", PROJECT_STATE_SOURCE)
    subprocess.run(["python3", "tools/update_project_progress.py"], cwd=ROOT, check=True)


def update_readme() -> None:
    text = read("README.md")
    text = replace_indented_marker(text, "Latest completed implementation phase:", DONE_PHASE)
    text = replace_indented_marker(text, "Current documentation consolidation:", DONE_PHASE)
    text = replace_indented_marker(text, "Next major implementation milestone:", NEXT_PHASE)
    text = replace_indented_marker(text, "Required planned follow-up:", FOLLOW_UP)
    text = ensure_item_after(text, "- SearchTimer Runtime Mutation Policy", "- Real VDR Acceptance Foundation\n- Daemon Runtime Lifecycle Hardening\n- Documentation Handoff Verification")
    text = text.replace("- SearchTimer Preview EPG Performance", "- " + CURRENT_FOUNDATION, 1)
    text = ensure_item_after(text, "- Runtime diagnostics", "- Real VDR acceptance manifest and runner\n- HTTP listener bind-failure hardening\n- SIGTERM daemon shutdown hardening")
    text = ensure_item_after(text, "- SearchTimer preview comparison options verified against live VDR EPG input", "- SearchTimer workflow validation and planning covered by real VDR acceptance")
    write("README.md", text)


def update_current_status() -> None:
    text = read("docs/development/current-status.md")
    text = replace_fenced_marker(text, "Latest completed implementation phase:", DONE_PHASE)
    text = replace_fenced_marker(text, "Current documentation consolidation state:", DONE_PHASE)
    text = replace_fenced_marker(text, "Next major implementation milestone:", NEXT_PHASE)
    text = replace_fenced_marker(text, "Required planned follow-up:", FOLLOW_UP)
    text = ensure_item_after(text, "SearchTimer Discovery Shared Decoder Cleanup", "Real VDR Acceptance Foundation\nDaemon Runtime Lifecycle Hardening\nDocumentation Handoff Verification")
    text = replace_fenced_marker(text, "Current foundation in progress:", CURRENT_FOUNDATION)
    write("docs/development/current-status.md", text)


def update_development_index() -> None:
    text = read("docs/development/index.md")
    text = replace_fenced_marker(text, "Current completed phase:", DONE_PHASE)
    text = replace_fenced_marker(text, "Next implementation focus:", NEXT_PHASE)
    text = ensure_item_after(text, "- [Phase 55.4d - SearchTimer Discovery Shared Decoder Cleanup](phase-55.4d-searchtimer-discovery-shared-decoder.md)", "- [Phase 55.5m - Documentation Consolidation and Roadmap Alignment](phase-55.5m-documentation-consolidation.md)")
    write("docs/development/index.md", text)


def update_dashboard() -> None:
    text = read("docs/project-status-dashboard.md")
    text = replace_fenced_marker(text, "Current Major Phase:", DONE_PHASE)
    text = replace_fenced_marker(text, "Current Documentation Consolidation:", DONE_PHASE)
    text = replace_fenced_marker(text, "Next Major Implementation Milestone:", NEXT_PHASE)
    text = replace_fenced_marker(text, "Required Planned Follow-Up:", FOLLOW_UP)
    text = ensure_item_after(text, "SearchTimer Preview EPG Cache   ADR-0034 documented", "Real VDR Acceptance             20/20 safe/dry-run probes verified\nDaemon Lifecycle Hardening       duplicate bind + SIGTERM verified")
    text = ensure_item_after(text, "SearchTimer Warm EPG Cache Architecture", "Real VDR Acceptance Foundation\nDaemon Runtime Lifecycle Hardening\nDocumentation Handoff Verification")
    text = text.replace("SearchTimer Preview EPG Performance", CURRENT_FOUNDATION, 1)
    text = ensure_item_after(text, "Phase 54.2  - SearchTimer warm EPG cache architecture", "Phase 55.5  - Real VDR acceptance and daemon lifecycle hardening\nPhase 55.5m - Documentation consolidation and roadmap alignment")
    write("docs/project-status-dashboard.md", text)


def update_roadmap() -> None:
    text = read("docs/planning/roadmap.md")
    current_position = """## Current Position

```text
Completed implementation state
Phase 55.5m - Documentation consolidation and roadmap alignment

Documentation consolidation step
Phase 55.5m - Documentation consolidation and roadmap alignment

Next major implementation milestone
Phase 55.6 - Recording operations audit and safety policy
```
"""
    text, count = re.subn(r"## Current Position\n\n```text\n.*?\n```\n", current_position, text, count=1, flags=re.S)
    if count != 1:
        raise SystemExit("roadmap current position block not found")

    recent_block = """## Recently Completed Technical Milestones

### Real VDR Acceptance Foundation

Status: Completed.

Primary result:
- Adds a repeatable real VDR acceptance manifest and runner for safe and dry-run API validation against a live daemon.

Key outcomes:
- Safe and dry-run acceptance scope currently passes 20/20 probes.
- Acceptance route validation prevents stale manifest paths.
- Per-probe timeout support handles cold large-recording queries without hiding unrelated probe behavior.
- JSON report output records machine-readable acceptance evidence.

Representative phase range:
- Phase 55.5c through Phase 55.5k.

---

### Daemon Runtime Lifecycle Hardening

Status: Completed.

Primary result:
- Makes daemon startup and shutdown behavior operator-safe for the current simple HTTP listener runtime.

Key outcomes:
- Duplicate daemon starts on an occupied HTTP port exit with status 1 instead of aborting.
- SIGTERM stops the daemon cleanly without `kill -9`.
- Port 18080 is released after normal shutdown.
- Guardrails cover HTTP bind failure handling and daemon runtime shutdown resets.

Representative phase range:
- Phase 55.4e and Phase 55.5l.

---

### Documentation Consolidation and Handoff Verification

Status: Completed.

Primary result:
- Aligns high-level project documentation with the verified Phase 55.5 runtime and acceptance state.

Key outcomes:
- Misleading percentage-style progress is replaced by a verified project state snapshot.
- README, roadmap, project state, current status, project dashboard, development index and completed-phase history now point at the same completed and next milestones.
- New-chat handoff rules preserve required local, real VDR and GitHub Actions checks.

Representative phase:
- Phase 55.5m.

---

"""
    if "## Recently Completed Technical Milestones" not in text:
        text = text.replace("## Current Milestone", recent_block + "## Current Milestone", 1)

    current_milestone = """## Current Milestone

### Phase 55.6 - Recording Operations Audit and Safety Policy

Status: Planned.

Goal:
- Audit live recording operations before enabling or expanding real write-capable paths.

Planned outcomes:
- Confirm which recording operations remain read-only, dry-run-only or destructive.
- Define operator confirmation, permission, backend allowlist and safety-policy requirements.
- Keep destructive real VDR probes explicitly opt-in.
- Preserve the current real VDR acceptance suite as safe/dry-run by default.
- Prepare the later recording operations implementation without silently opening mutation paths.
"""
    text, count = re.subn(r"## Current Milestone\n\n### .*?(?=\n---\n|\Z)", current_milestone, text, count=1, flags=re.S)
    if count != 1:
        raise SystemExit("roadmap current milestone section not found")
    write("docs/planning/roadmap.md", text)


def update_completed_phases() -> None:
    text = read("docs/development/completed-phases.md")
    if "### Real VDR Acceptance and Runtime Lifecycle Foundation" not in text:
        text = text.replace("### Documentation Consolidation", COMPLETED_OVERVIEW_BLOCK + "### Documentation Consolidation", 1)
    if "## Phase 55.5m - Documentation consolidation and roadmap alignment" not in text:
        text = text.replace("## Detailed Phase History\n", "## Detailed Phase History\n\n" + DETAILED_PHASE_BLOCK, 1)
    text = text.replace("Status: In Progress.\n\nPurpose:\n- Refresh the high-level project documentation before starting the next major milestone.", "Status: Completed.\n\nPurpose:\n- Refresh the high-level project documentation before starting the next major milestone.", 1)
    text = text.replace("Remaining planned documentation work:\n- Completed phase milestone restructuring.\n- Current status refresh.\n- README refresh.", "Completed consolidation work:\n- Completed phase milestone restructuring.\n- Current status refresh.\n- README refresh.\n- Roadmap, dashboard and project state alignment.\n- New-chat handoff verification checklist.", 1)
    write("docs/development/completed-phases.md", text)


def write_consolidation_doc() -> None:
    write("docs/development/phase-55.5m-documentation-consolidation.md", CONSOLIDATION_DOC)


def main() -> int:
    update_project_state_source()
    update_readme()
    update_current_status()
    update_development_index()
    update_dashboard()
    update_roadmap()
    update_completed_phases()
    write_consolidation_doc()
    print("Phase 55.5m documentation consolidation applied with verified project state snapshot.")
    print("Next checks:")
    print("  make test-docs")
    print("  make test-phase")
    print("  make test-real-vdr-acceptance-manifest")
    print("  make test-daemon-runtime-shutdown-resets")
    print("  make test-http-listener-bind-failure-handling")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
