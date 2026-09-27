#!/usr/bin/env python3
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "closeout": ROOT / "docs/development/phase-69c-closeout.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "completed": ROOT / "docs/development/completed-phases-latest.md",
    "handoff": ROOT / "docs/NEW-CHAT-HANDOFF.md",
    "productive": ROOT / "docs/development/phase-69c-native-timer-create-productive-runtime.md",
}
def read(name):
    p=FILES[name]
    if not p.is_file(): raise SystemExit(f"missing Phase 69.C closeout file: {p.relative_to(ROOT)}")
    return p.read_text(encoding="utf-8")
def require(text, token, label):
    if token not in text: raise SystemExit(f"missing {label}: {token}")
def forbid(text, token, label):
    if token in text: raise SystemExit(f"stale {label}: {token}")
closeout=read("closeout")
for token in ("**COMPLETED**","69.C=COMPLETED","69.D - Collections, pagination and partial results","PR #361","9316fb48a49f3e61ff89d3c01a225915003fa8e8","a38e2363929c2a1f1b60984f6176aa88446a9054","CI #9285","PR361_REAL_YAVDR_ACCEPTANCE=PASS","DURABLE_RESTART_NO_REDISPATCH=PASS","idempotency_conflict","revision_conflict","ADR-0064 remains closed"):
    require(closeout,token,"Phase 69.C closeout evidence")
current=read("current")
require(current,"69.E - Compatibility and deprecation policy","CURRENT downstream active slice")
require(current,"69.C=COMPLETED","CURRENT completed 69.C marker")
require(current,"[Phase 69.C Closeout]","CURRENT closeout navigation")
forbid(current,"Current active runtime slice:\n69.C -","CURRENT active 69.C marker")
forbid(current,"Current bounded 69.C step:","CURRENT bounded 69.C marker")
status=read("status")
require(status,"Current active runtime slice: **69.E - Compatibility and deprecation policy**","current-status downstream slice")
require(status,"[Phase 69.C Closeout]","current-status closeout link")
roadmap=read("roadmap")
require(roadmap,"Status: **Active — 69.E Compatibility and deprecation policy.**","roadmap Phase 69 status")
require(roadmap,"Status: **Completed.** Durable evidence: [Phase 69.C Closeout]","roadmap 69.C completion")
require(roadmap,"#### 69.D — Collections, pagination and partial results\n\nStatus: **Completed.** Durable evidence: [Phase 69.D Closeout]","roadmap 69.D completion")
forbid(roadmap,"Phase 69 has not started","roadmap pre-start Phase 69 marker")
forbid(roadmap,"Phase 69 is next but has not started","roadmap pre-start Phase 69 prose")
phase_map=read("phase_map")
require(phase_map,"| 6 | Phase 69 | Active — 69.E |","phase-map downstream row")
completed=read("completed")
require(completed,"Current slice: 69.E - Compatibility and deprecation policy","completed-phases current slice")
require(completed,"Accepted slices: 69.A, 69.B, 69.C, 69.D","completed-phases accepted slices")
handoff=read("handoff")
require(handoff,"Current active runtime slice: **69.E - Compatibility and deprecation policy**","handoff downstream slice")
require(handoff,"69.A, 69.B, 69.C and 69.D are accepted","handoff accepted 69.C and downstream 69.D")
require(handoff,"Treat Phase 69 - Public API and Client Compatibility Hardening as the active numbered runtime phase","handoff active Phase 69 instruction")
forbid(handoff,"Treat Phase 68 - Legacy OSD Compatibility Bridge as the active numbered runtime phase","handoff stale Phase 68 instruction")
productive=read("productive")
require(productive,"**ACCEPTED — productive native-effect Timer CREATE boundary closed by PR #361.**","productive runtime accepted state")
require(productive,"69.C=COMPLETED","productive runtime completed phase marker")
require(productive,"[Phase 69.C Closeout]","productive runtime closeout link")
print("Phase 69.C closeout guard passed.")
