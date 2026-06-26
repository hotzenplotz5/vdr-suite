#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def p(relative: str) -> Path:
    return ROOT / relative


def read(relative: str) -> str:
    return p(relative).read_text(encoding="utf-8")


def write(relative: str, text: str) -> None:
    p(relative).write_text(text, encoding="utf-8")


def replace_all(relative: str, old: str, new: str) -> None:
    text = read(relative)
    if old in text:
        text = text.replace(old, new)
        write(relative, text)


def replace_once(relative: str, old: str, new: str) -> None:
    text = read(relative)
    if old not in text:
        return
    text = text.replace(old, new, 1)
    write(relative, text)


def insert_after(relative: str, marker: str, addition: str, guard: str) -> None:
    text = read(relative)
    if guard in text:
        return
    if marker not in text:
        raise SystemExit(f"marker not found in {relative}: {marker}")
    text = text.replace(marker, marker + addition, 1)
    write(relative, text)


# Phase markers used by tools/check_phase_consistency.py
for relative in [
    "docs/development/current-status.md",
    "docs/project-status-dashboard.md",
    "docs/planning/roadmap.md",
]:
    replace_all(
        relative,
        "Phase 54.0 - SearchTimer runtime mutation policy wiring",
        "Phase 54.2 - SearchTimer warm EPG cache architecture",
    )
    replace_all(
        relative,
        "Phase 54.1 - SearchTimer operator-controlled runtime mutation enablement",
        "Phase 54.3 - SearchTimer warm EPG cache implementation",
    )

# Current status detail updates
insert_after(
    "docs/development/current-status.md",
    "- Phase 53.8 closes the SearchTimer title-only workflow chain with a completion audit and keeps production mutation gated.\n",
    "- Phase 54.1 fixes SearchTimer preview comparison-option handling and verifies title-only preview behavior against live VDR EPG input.\n"
    "- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.\n",
    "Phase 54.2 accepts ADR-0034",
)

insert_after(
    "docs/development/current-status.md",
    "- EPG search operates over selective event windows and does not require a persistent full EPG mirror.\n",
    "- SearchTimer preview must use warm EPG input for interactive operation and must not hide missing EPG input as a normal zero-match result.\n",
    "SearchTimer preview must use warm EPG input",
)

replace_once(
    "docs/development/current-status.md",
    "The next implementation phase should begin the cross-backend search and federation foundation with planning-only architecture documentation.",
    "The next implementation phase should implement a backend-scoped warm EPG cache for SearchTimer preview so interactive preview requests do not fetch the full RESTfulAPI EPG dump.",
)

replace_once(
    "docs/development/current-status.md",
    "- prefer selective EPG queries over persistent full EPG mirroring",
    "- prefer selective EPG queries and warm cache refresh over full EPG transfers in interactive request paths",
)

# Dashboard detail update
insert_after(
    "docs/project-status-dashboard.md",
    "SearchTimer User Workflow       completed foundation + verified execution\n",
    "SearchTimer Preview EPG Cache   architecture documented\n",
    "SearchTimer Preview EPG Cache",
)

# Roadmap phase block update
replace_once(
    "docs/planning/roadmap.md",
    "### Phase 54 - Cross-Backend Search and Federation\n\nStatus: Planned.\n\nGoal:\n- Make multi-backend search first-class across recordings, EPG and metadata.\n\nExpected outcomes:\n- Cross-backend result aggregation.\n- Backend-specific result context.\n- Conflict and duplicate handling.\n- Remote backend visibility rules.\n",
    "### Phase 54 - SearchTimer Preview Runtime and EPG Input Performance\n\nStatus: In Progress.\n\nGoal:\n- Make SearchTimer preview correct and fast enough for operator-facing use against real VDR EPG data.\n\nCompleted results:\n- Phase 54.0 wires runtime mutation policy execution while keeping the runtime gate closed by default.\n- Phase 54.1 fixes SearchTimer preview comparison options and verifies title-only matching against live VDR EPG input.\n- Phase 54.2 accepts ADR-0034 for warm EPG cache input, change-state invalidation and future SSE-triggered refresh.\n\nNext implementation step:\n- Phase 54.3 - SearchTimer warm EPG cache implementation.\n\nExpected outcomes:\n- Backend-scoped warm EPG cache for SearchTimer preview.\n- Explicit cache readiness and stale-state diagnostics.\n- No full RESTfulAPI EPG dump per interactive preview request.\n- Change-state driven cache invalidation through polling first.\n- Future RESTfulAPI SSE change stream support as an invalidation signal.\n",
)

replace_once(
    "docs/planning/roadmap.md",
    "### Phase 54.0 result: SearchTimer runtime mutation policy wiring\n\n- Runtime Create/Update/Delete services are available to the SearchTimer controller.\n- Direct API mutation is guarded by a closed runtime policy executor.\n- The next phase can add explicit operator-controlled enablement and a safe live create/delete smoke test.\n",
    "### Phase 54.0 result: SearchTimer runtime mutation policy wiring\n\n- Runtime Create/Update/Delete services are available to the SearchTimer controller.\n- Direct API mutation is guarded by a closed runtime policy executor.\n\n### Phase 54.1 result: SearchTimer preview comparison-option fix\n\n- SearchTimerPreviewService now respects compareTitle, compareSubtitle and compareSummary.\n- SearchTimer preview now respects limit and offset.\n- Real yaVDR validation confirmed title-only SearchTimer preview produces the expected Treffer set for \"Amerika\".\n\n### Phase 54.2 result: SearchTimer warm EPG cache architecture\n\n- ADR-0034 defines that interactive SearchTimer preview must not fetch the full RESTfulAPI EPG dump per request.\n- The next implementation step is a backend-scoped warm EPG cache with explicit readiness diagnostics.\n- Polling-based change-state invalidation comes first; RESTfulAPI SSE remains a later invalidation trigger.\n",
)

# Completed phases: insert the two latest records near the top so latest completed heading is Phase 54.2.
insert_after(
    "docs/development/completed-phases.md",
    "- README refresh.\n",
    "\n## Phase 54.1 - SearchTimer preview comparison-option fix\n\nStatus: Completed.\n\nSummary:\n- Fixed SearchTimerPreviewService so preview respects compareTitle, compareSubtitle and compareSummary.\n- Fixed preview limit and offset behavior.\n- Added targeted test coverage for title-only, subtitle-only, summary-only, limit and offset behavior.\n- Verified against real yaVDR EPG input that title-only \"Amerika\" matching returns the expected SearchTimer preview result.\n- Kept backend mutation out of scope.\n\n---\n\n## Phase 54.2 - SearchTimer warm EPG cache architecture\n\nStatus: Completed.\n\nSummary:\n- Added ADR-0034 for SearchTimer warm EPG cache and change invalidation.\n- Documented the measured RESTfulAPI full EPG dump cost of about 30 MB and about 28 seconds for a 14-day EPG window.\n- Decided that interactive SearchTimer preview must not fetch the full RESTfulAPI EPG dump per request.\n- Defined backend-scoped warm EPG cache readiness, stale-state and future SSE invalidation rules.\n- Updated roadmap, project status and development documentation to make Phase 54.3 the warm EPG cache implementation step.\n\n---\n",
    "## Phase 54.2 - SearchTimer warm EPG cache architecture",
)

print("Phase 54.2 documentation finalization complete.")
