#!/usr/bin/env python3
"""Guard the final Phase 69 public API/client compatibility closeout."""

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = ROOT / path
    if not p.is_file():
        raise SystemExit("missing Phase 69 closeout dependency: " + path)
    return p.read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

closeout = read("docs/development/phase-69-closeout.md")
for token in (
    "**Phase 69 is completed.**",
    "phase69=COMPLETED",
    "69.F=COMPLETED",
    "Phase 70 - Recommendation and Content Knowledge Graph [NOT STARTED]",
    "PR #381",
    "f8034648907a02965c41cbf49be24b14f3e38356",
    "4171574f76eeb2d1a7aad9861a81de90451b820a",
    "CI #9357 / run 36369528246 = SUCCESS (6/6)",
    "8 stable public-v1 contracts",
    "0 uncovered stable contracts",
    "exact-head real yaVDR acceptance",
    "This closeout is **not** authorization to implement Phase 70.",
):
    require(token in closeout, "Phase 69 closeout evidence drifted: " + token)

for path, marker in (
    ("docs/development/phase-69f-client-error-mutation-fallback.md", "**ACCEPTED — first bounded 69.F slice via PR #369.**"),
    ("docs/development/phase-69f-searchtimer-fallback-removal.md", "**ACCEPTED — second bounded 69.F slice via PR #370.**"),
    ("docs/development/phase-69f-read-alias-fallback-removal.md", "**ACCEPTED — third bounded 69.F slice via PR #371.**"),
    ("docs/development/phase-69f-public-v1-operation-reference-client.md", "**ACCEPTED — thirteenth bounded 69.F slice via PR #381.**"),
):
    require(marker in read(path), path + " acceptance state drifted")

matrix = json.loads(read("docs/development/phase-69f-client-contract-matrix.json"))
resources = {
    item["method"] + " " + item["template"]
    for item in matrix.get("publicV1Resources", [])
}
references = matrix.get("publicClientReferences", [])
covered = {
    contract
    for reference in references
    for contract in reference.get("resources", [])
}
require(len(resources) >= 8, "Phase 69 closeout baseline of eight stable public-v1 contracts disappeared")
require(covered == resources, "reference coverage must equal the stable public-v1 set exactly")
require(all(r.get("status") == "accepted" for r in references if r.get("id") != "post-phase69-vidaa-recording-collection"), "Phase-69 public reference slices must remain accepted")
require(all(r.get("status") in ("accepted", "candidate") for r in references), "new public reference slices require explicit status")
require(matrix.get("explicitDeferredFallbacks") == [], "explicit browser fallback debt must be empty")
candidate = matrix.get("derivedNextRuntimeCandidate", {})
require(candidate.get("domain") == "phase69-complete", "matrix must mark Phase 69 complete")
require(candidate.get("proposedTemplate") is None, "Phase 69 closeout must not invent a successor route")

current = read("docs/CURRENT.md")
for token in (
    "Latest completed numbered runtime phase:\nPhase 69 - Public API and Client Compatibility Hardening",
    "Current active numbered runtime phase:\nnone - Phase 70 - Recommendation and Content Knowledge Graph not started",
    "Next strict numbered runtime phase:\nPhase 70 - Recommendation and Content Knowledge Graph",
    "[Phase 69 Closeout](development/phase-69-closeout.md)",
):
    require(token in current, "CURRENT Phase 69 closeout status drifted: " + token)

roadmap = read("docs/planning/roadmap.md")
require(
    "## Phase 69 — Public API and Client Compatibility Hardening\n\nStatus: **Completed.**" in roadmap,
    "roadmap must mark Phase 69 Completed",
)
require(
    "#### 69.F — First-party and third-party client hardening\n\nStatus: **Completed.**" in roadmap,
    "roadmap must mark 69.F Completed",
)
require(
    "## Phase 70 — Recommendation and Content Knowledge Graph\n\nStatus: **Next; not started.**" in roadmap,
    "roadmap must leave Phase 70 not started",
)
require("**Gate status: satisfied.**" in roadmap, "roadmap Phase-69 gate must be satisfied")

phase_map = read("docs/planning/phase-map.md")
require("| 6 | Phase 69 | Completed |" in phase_map, "phase map must mark Phase 69 completed")
require("| 7 | Phase 70 | Next — not started |" in phase_map, "phase map must leave Phase 70 not started")

for path in (
    "docs/development/current-status.md",
    "docs/NEW-CHAT-HANDOFF.md",
):
    text = read(path)
    require(
        "none - Phase 70 - Recommendation and Content Knowledge Graph not started" in text,
        path + " must show no active numbered runtime phase",
    )
    require("Phase 70 - Recommendation and Content Knowledge Graph" in text, path + " must name the next strict phase")

require(
    "Phase 69 - Public API and Client Compatibility Hardening" in read("docs/development/completed-phases-latest.md"),
    "latest completed marker must include Phase 69",
)
require(
    "| Phase 69 | Completed |" in read("docs/development/completed-phases.md"),
    "completed phases table must include Phase 69",
)
require(
    "| Stable public API/SDK | Completed numbered domain |" in read("docs/project-status-dashboard.md"),
    "project dashboard must mark stable public API completed",
)
require(
    "[Phase 69 Closeout](../development/phase-69-closeout.md)" in read("docs/planning/index.md"),
    "planning index must link Phase 69 closeout",
)
require(
    "[Phase 69 Closeout](phase-69-closeout.md)" in read("docs/development/index.md"),
    "development index must link Phase 69 closeout",
)
require(
    "[Phase 69 Closeout](development/phase-69-closeout.md)" in read("docs/index.md"),
    "documentation index must link Phase 69 closeout",
)
require(
    "Phase 69 - Public API and Client Compatibility Hardening [COMPLETED]" in read("ROADMAP.md"),
    "root roadmap must mark Phase 69 completed",
)
require(
    "[Phase 69 Closeout](phase-69-closeout.md)" in read("docs/development/phase-69-public-api-kickoff.md"),
    "Phase 69 implementation record must link final closeout",
)

phase_make = read("mk/phase69-public-api-tests.mk")
require("test-phase69-closeout:" in phase_make, "Phase 69 closeout Make target missing")
require("python3 tools/check_phase69_closeout.py" in phase_make, "Phase 69 closeout guard command missing")
maintenance = read("mk/maintenance-tests.mk")
require("python3 tools/check_phase69_closeout.py" in maintenance, "architecture group must run Phase 69 closeout guard")
require("test-phase69-closeout" in maintenance, "phase group must include Phase 69 closeout")
require("test-phase69-closeout" in read("mk/test-groups.mk"), "fast CI must include Phase 69 closeout")

print("Phase 69 closeout guard passed.")
print("69.A-F are completed; the eight-contract Phase-69 baseline remains present and current stable public-v1 reference coverage stays exact.")
print("Phase 70 remains next but not started and requires its own accepted runtime ADR.")
