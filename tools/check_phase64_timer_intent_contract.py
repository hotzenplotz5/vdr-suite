#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerIntent.h",
    "core/timers/src/TimerIntent.cpp",
    "core/timers/tests/test_timer_intent.cpp",
    "docs/development/phase-64-timer-intent-contract.md",
    "mk/phase64-timer-intent-tests.mk",
    "Makefile",
    "docs/planning/roadmap.md",
]
for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Phase-64 TimerIntent contract file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")
makefile = (ROOT / required_files[5]).read_text(encoding="utf-8")
roadmap = (ROOT / required_files[6]).read_text(encoding="utf-8")

for token in [
    "TimerIntentType",
    "TimerIntentState",
    "TimerIntentSpec",
    "TimerIntent",
    "TimerIntentAutomationSource",
    "TimerIntentBackendEventRef",
    "TimerIntentChannelRequirement",
    "TimerIntentSchedule",
    "TimerIntentRecordingOptions",
    "TimerIntentAssignmentPolicy",
    "TimerIntentReplicaPolicy",
    "TimerIntentDuplicatePolicy",
    "timerIntentRevisionMatches",
    "timerIntentCanTransition",
    "timerIntentAssignable",
    "timerIntentTerminal",
    "timerIntentSemanticIdentity",
]:
    if token not in header:
        raise SystemExit(f"missing TimerIntent header contract marker: {token}")

for token in [
    '"programme_event"',
    '"manual_window"',
    '"recurring_schedule"',
    '"draft"',
    '"active"',
    '"paused"',
    '"satisfied"',
    '"cancel_requested"',
    '"cancelled"',
    '"expired"',
    '"failed"',
    '"timer-intent-semantic/1|"',
    "appendField",
    "desiredAssignments",
    "programEventId",
    "backendEventRef",
    "channelRequirement",
]:
    if token not in source:
        raise SystemExit(f"missing TimerIntent source contract marker: {token}")

for token in [
    "timerIntentValidSpec(base)",
    "timerIntentRevisionMatches",
    "timerIntentCanTransition",
    "timerIntentSemanticIdentity(collisionLeft) != timerIntentSemanticIdentity(collisionRight)",
    "replicaPolicy.desiredAssignments = 2",
    "assignmentPolicy.allowFailover = false",
    "duplicatePolicy.requireOperatorReviewOnAmbiguity = false",
]:
    if token not in test:
        raise SystemExit(f"missing TimerIntent regression marker: {token}")

for token in [
    "Control Plane",
    "TimerAssignment",
    "NativeTimerBinding",
    "SearchTimer",
    "timer-intent-semantic/1|",
    "programme_event",
    "manual_window",
    "recurring_schedule",
    "authoritative readback",
    "mutations=disabled",
    "no real yaVDR acceptance",
    "TimerIntent persistence",
]:
    if token not in doc:
        raise SystemExit(f"missing Phase-64 contract statement: {token}")

for token in [
    "test-phase64-timer-intent-contract-architecture",
    "test-phase64-timer-intent-contract:",
    "core/timers/src/TimerIntent.cpp",
    "core/timers/tests/test_timer_intent.cpp",
    "test-fast: test-phase64-timer-intent-contract",
    "test-architecture: test-phase64-timer-intent-contract-architecture",
]:
    if token not in make_fragment:
        raise SystemExit(f"missing Phase-64 test-graph marker: {token}")

if "include mk/phase64-timer-intent-tests.mk" not in makefile:
    raise SystemExit("Phase-64 TimerIntent test fragment is not included by Makefile")

for token in [
    "Phase 63 - Backend Agent and Secure Multi-Site Runtime",
    "Phase 64 - Timer Intent and Multi-Backend Orchestration",
    "Phase 64 Slice 1 - TimerIntent Domain Contract",
    "Status: **Completed.**",
    "Status: **Active; Slice 1 is the TimerIntent domain contract.**",
    "No TimerIntent persistence; no TimerAssignment; no NativeTimerBinding",
]:
    if token not in roadmap:
        raise SystemExit(f"missing Phase-64 roadmap boundary: {token}")

# Slice 1 is a pure domain contract. Any TimerIntent production wiring outside
# core/timers at this point would skip the separately reviewed persistence,
# assignment and native-mutation slices.
forbidden_roots = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core" / "agent",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "runtime",
    ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]
text_suffixes = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk", ".conf", ".service"}
for root in forbidden_roots:
    if not root.exists():
        continue
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "TimerIntent" in text or "timer-intent-semantic/1" in text:
            raise SystemExit(f"premature Phase-64 TimerIntent runtime wiring: {path.relative_to(ROOT)}")

for relative in [required_files[0], required_files[1], required_files[2], required_files[4]]:
    text = (ROOT / relative).read_text(encoding="utf-8")
    if "mutations=enabled" in text:
        raise SystemExit(f"Phase-64 contract must not enable mutations: {relative}")

print("Phase-64 TimerIntent contract check passed")
print("Contract boundary: TimerIntent only; assignment/native runtime deferred")
