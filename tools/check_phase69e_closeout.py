#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "closeout": ROOT / "docs/development/phase-69e-closeout.md",
    "foundation": ROOT / "docs/development/phase-69e-compatibility-policy-foundation.md",
    "classification": ROOT / "docs/development/phase-69e-legacy-route-classification.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "planning_index": ROOT / "docs/planning/index.md",
    "dashboard": ROOT / "docs/project-status-dashboard.md",
    "completed": ROOT / "docs/development/completed-phases-latest.md",
    "handoff": ROOT / "docs/NEW-CHAT-HANDOFF.md",
    "kickoff": ROOT / "docs/development/phase-69-public-api-kickoff.md",
    "compat_guard": ROOT / "tools/check_phase69e_compatibility_policy.py",
    "classification_guard": ROOT / "tools/check_phase69e_legacy_route_classification.py",
    "runtime": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "phase_make": ROOT / "mk/phase69-public-api-tests.mk",
    "groups": ROOT / "mk/test-groups.mk",
    "maintenance": ROOT / "mk/maintenance-tests.mk",
}

def read(name):
    path = FILES[name]
    if not path.is_file():
        raise SystemExit(
            f"missing Phase 69.E closeout file: {path.relative_to(ROOT)}"
        )
    return path.read_text(encoding="utf-8")

def require(text, token, label):
    if token not in text:
        raise SystemExit(f"missing {label}: {token}")

def require_any(text, tokens, label):
    if not any(token in text for token in tokens):
        raise SystemExit(f"missing {label}: one of {tokens}")

def forbid(text, token, label):
    if token in text:
        raise SystemExit(f"stale {label}: {token}")

closeout = read("closeout")
for token in (
    "**COMPLETED**",
    "69.E=COMPLETED",
    "next=69.F - First-party and third-party client hardening",
    "PR #366",
    "e205be80f07c5e9f20c6b4a8747ae89b850708f2",
    "42c2d9fe0bf453f0dceddb77e85eaf4704ab6d1e",
    "CI #9303",
    "36314288241",
    "PR #367",
    "7a2ddd8ce24a9c2876f12b7aeea2da28bdc9f2cf",
    "11e6272604f7ed827f7671fcf85276bcebdce346",
    "CI #9308",
    "36316653141",
    "118 retained unversioned route literals",
    "27 proven same-handler alias groups",
    "64 standalone transition literals",
    "0 deprecated aliases",
    "deprecatedAliases = []",
    "15 wrapper fallback call sites",
    "four definite state-changing SearchTimer mutation paths",
    "69.F - First-party and third-party client hardening [ACTIVE]",
    "Home / Recording fanout / LiveTV / Timer/native ownership unchanged",
):
    require(closeout, token, "Phase 69.E closeout evidence")

foundation = read("foundation")
require(
    foundation,
    "**ACCEPTED — first bounded 69.E slice.**",
    "accepted compatibility-policy foundation",
)
require(foundation, "[Phase 69.E Closeout]", "foundation closeout link")

classification = read("classification")
require(
    classification,
    "**ACCEPTED — second bounded 69.E slice.**",
    "accepted legacy-route classification",
)
require(classification, "[Phase 69.E Closeout]", "classification closeout link")

runtime = read("runtime")
require(runtime, r'\"deprecatedAliases\":[]', "empty deprecatedAliases capability")

compat_guard = read("compat_guard")
for token in (
    "public compatibility discovery contract",
    "compatibility regression coverage",
    "documented 69.E foundation",
):
    require(compat_guard, token, "retained compatibility-policy guard")

classification_guard = read("classification_guard")
for token in (
    "Phase 69.E legacy-route classification guard passed.",
    "15 wrapper call sites",
    "four definite state-changing SearchTimer mutation fallbacks",
):
    require(classification_guard, token, "retained classification guard")

current = read("current")
require(current, "[Phase 69.E Closeout]", "CURRENT 69.E closeout link")
require_any(
    current,
    (
        "Current active runtime slice:\n69.F - First-party and third-party client hardening",
        "[Phase 69 Closeout](development/phase-69-closeout.md)",
    ),
    "CURRENT 69.F successor/Phase-69 closeout",
)
forbid(
    current,
    "Current active runtime slice:\n69.E - Compatibility and deprecation policy",
    "CURRENT active 69.E marker",
)

status = read("status")
require(status, "[Phase 69.E Closeout]", "current-status 69.E closeout link")
require_any(
    status,
    (
        "Current active runtime slice: **69.F - First-party and third-party client hardening**",
        "[Phase 69 Closeout](phase-69-closeout.md)",
    ),
    "current-status 69.F successor/Phase-69 closeout",
)

roadmap = read("roadmap")
require_any(
    roadmap,
    (
        "Status: **Active — 69.F First-party and third-party client hardening.**",
        "Status: **Completed.** Durable evidence: [Phase 69 Closeout]",
    ),
    "roadmap Phase 69 active/completed status",
)
require(
    roadmap,
    "#### 69.E — Compatibility and deprecation policy\n\nStatus: **Completed.** Durable evidence: [Phase 69.E Closeout]",
    "roadmap 69.E completion",
)
require_any(
    roadmap,
    (
        "#### 69.F — First-party and third-party client hardening\n\nStatus: **Active.**",
        "#### 69.F — First-party and third-party client hardening\n\nStatus: **Completed.**",
    ),
    "roadmap 69.F active/completed status",
)

phase_map = read("phase_map")
require_any(
    phase_map,
    ("| 6 | Phase 69 | Active — 69.F |", "| 6 | Phase 69 | Completed |"),
    "phase-map Phase 69 active/completed row",
)

planning_index = read("planning_index")
require_any(
    planning_index,
    (
        "active at 69.F First-party and third-party client hardening",
        "completed numbered planning boundary is now Phase 69",
    ),
    "planning index Phase 69 transition",
)
require(planning_index, "[Phase 69.E Closeout]", "planning index 69.E closeout")

dashboard = read("dashboard")
require_any(
    dashboard,
    (
        "| Stable public API/SDK | Active — 69.F client hardening |",
        "| Stable public API/SDK | Completed numbered domain |",
    ),
    "dashboard Phase 69 status",
)

completed = read("completed")
require_any(
    completed,
    (
        "Current slice: 69.F - First-party and third-party client hardening",
        "Phase 69.A-F are completed and accepted",
    ),
    "completed-phases Phase 69 state",
)

handoff = read("handoff")
require_any(
    handoff,
    (
        "Current active runtime slice: **69.F - First-party and third-party client hardening**",
        "Current active numbered runtime slice: **none - Phase 70 - Recommendation and Content Knowledge Graph not started**",
        "[Phase 69 Closeout](development/phase-69-closeout.md)",
    ),
    "handoff 69.F successor state",
)
require(
    handoff,
    "docs/development/phase-69e-closeout.md",
    "handoff 69.E closeout prerequisite",
)

kickoff = read("kickoff")
require(kickoff, "[Phase 69.E Closeout]", "kickoff 69.E closeout link")
require_any(
    kickoff,
    (
        "current slice: 69.F First-party and third-party client hardening",
        "[Phase 69 Closeout](phase-69-closeout.md)",
    ),
    "kickoff 69.F successor/closeout state",
)

phase_make = read("phase_make")
require(phase_make, ".PHONY: test-phase69e-closeout", "closeout make target")
require(
    phase_make,
    "python3 tools/check_phase69e_closeout.py",
    "closeout guard command",
)

groups = read("groups")
require(groups, "test-phase69e-closeout", "closeout CI-fast wiring")

maintenance = read("maintenance")
require(maintenance, "python3 tools/check_phase69e_closeout.py", "architecture wiring")
require(maintenance, "test-phase69e-closeout", "phase wiring")

print("Phase 69.E closeout guard passed.")
print(
    "Boundary: PR #366 establishes public compatibility/deprecation discovery; "
    "PR #367 classifies every retained unversioned route without inventing a successor; "
    "69.F owns client migration and fallback hardening."
)
