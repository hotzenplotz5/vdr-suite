#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "closeout": ROOT / "docs/development/phase-69d-closeout.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "planning_index": ROOT / "docs/planning/index.md",
    "dashboard": ROOT / "docs/project-status-dashboard.md",
    "completed": ROOT / "docs/development/completed-phases-latest.md",
    "handoff": ROOT / "docs/NEW-CHAT-HANDOFF.md",
    "kickoff": ROOT / "docs/development/phase-69-public-api-kickoff.md",
    "timer_doc": ROOT / "docs/development/phase-69d-timer-assignment-collection.md",
    "channel_doc": ROOT / "docs/development/phase-69d-public-channel-federation.md",
    "timer_guard": ROOT / "tools/check_phase69d_timer_assignment_collection.py",
    "channel_guard": ROOT / "tools/check_phase69d_public_channel_collection.py",
    "phase_make": ROOT / "mk/phase69-public-api-tests.mk",
    "groups": ROOT / "mk/test-groups.mk",
}

def read(name):
    path = FILES[name]
    if not path.is_file():
        raise SystemExit(
            f"missing Phase 69.D closeout file: {path.relative_to(ROOT)}"
        )
    return path.read_text(encoding="utf-8")

def require(text, token, label):
    if token not in text:
        raise SystemExit(f"missing {label}: {token}")

def forbid(text, token, label):
    if token in text:
        raise SystemExit(f"stale {label}: {token}")

closeout = read("closeout")
for token in (
    "**COMPLETED**",
    "69.D=COMPLETED",
    "next=69.E - Compatibility and deprecation policy",
    "PR #363",
    "7c8e97b4b6e494a123a368395695459946630faa",
    "45ef8e3665082c2b1bb4e3e7f54c67e1e18833d6",
    "CI #9291",
    "36301143784",
    "PR #364",
    "f2c7ada38b80ba55e8d216401ed744b6fc3b02ba",
    "d39b1f6f52d2bc83458cabb7fb71bf4f8dac4890",
    "CI #9295",
    "36310001132",
    "meta.partial=true",
    "503 backend_unavailable",
    "69.E - Compatibility and deprecation policy [ACTIVE]",
):
    require(closeout, token, "Phase 69.D closeout evidence")

timer_doc = read("timer_doc")
require(timer_doc, "**ACCEPTED — first bounded 69.D collection slice", "accepted TimerAssignment collection")
require(timer_doc, "PR #363", "TimerAssignment PR evidence")
channel_doc = read("channel_doc")
require(channel_doc, "**ACCEPTED — second bounded 69.D collection slice", "accepted Channel collection")
require(channel_doc, "PR #364", "Channel PR evidence")
require(channel_doc, "[Phase 69.D Closeout]", "Channel closeout link")

timer_guard = read("timer_guard")
for token in (
    "timerAssignmentId ASC",
    "PublicTimerAssignmentDefaultLimit = 50U",
    "PublicTimerAssignmentMaximumLimit = 100U",
):
    require(timer_guard, token, "retained TimerAssignment collection guard")

channel_guard = read("channel_guard")
for token in (
    "PublicChannelMaximumSources = 16U",
    "backendId,channelId",
    "backend_unavailable",
    "partial",
):
    require(channel_guard, token, "retained Channel federation guard")

current = read("current")
require(current, "[Phase 69.D Closeout]", "CURRENT 69.D closeout link")
forbid(
    current,
    "Current active runtime slice:\n69.D - Collections, pagination and partial results",
    "CURRENT active 69.D marker",
)

status = read("status")
require(status, "[Phase 69.D Closeout]", "current-status 69.D closeout link")

roadmap = read("roadmap")
require(
    roadmap,
    "#### 69.D — Collections, pagination and partial results\n\nStatus: **Completed.** Durable evidence: [Phase 69.D Closeout]",
    "roadmap 69.D completion",
)

planning_index = read("planning_index")
require(planning_index, "[Phase 69.D Closeout]", "planning index closeout")

handoff = read("handoff")
require(
    handoff,
    "docs/development/phase-69d-closeout.md",
    "handoff 69.D closeout prerequisite",
)

kickoff = read("kickoff")
require(kickoff, "[Phase 69.D Closeout]", "kickoff closeout link")

phase_make = read("phase_make")
require(phase_make, ".PHONY: test-phase69d-closeout", "closeout make target")
require(
    phase_make,
    "python3 tools/check_phase69d_closeout.py",
    "closeout guard command",
)

groups = read("groups")
require(groups, "test-phase69d-closeout", "closeout CI wiring")

print("Phase 69.D closeout guard passed.")
print(
    "Boundary: PR #363 proves standard single-source collection/keyset semantics; "
    "PR #364 proves federated partial-source semantics; later Phase-69 slices may advance independently."
)
