#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerIntent.h",
    "core/timers/src/TimerIntent.cpp",
    "core/timers/tests/test_timer_intent.cpp",
    "core/timers/include/TimerIntentRepository.h",
    "core/timers/src/TimerIntentRepository.cpp",
    "core/timers/tests/test_timer_intent_repository.cpp",
    "docs/development/phase-64-timer-intent-contract.md",
    "docs/development/phase-64-timer-intent-repository.md",
    "mk/phase64-timer-intent-tests.mk",
    "Makefile",
    "docs/planning/roadmap.md",
]
for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Phase-64 TimerIntent file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
repository_header = (ROOT / required_files[3]).read_text(encoding="utf-8")
repository_source = (ROOT / required_files[4]).read_text(encoding="utf-8")
repository_test = (ROOT / required_files[5]).read_text(encoding="utf-8")
contract_doc = (ROOT / required_files[6]).read_text(encoding="utf-8")
repository_doc = (ROOT / required_files[7]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[8]).read_text(encoding="utf-8")
makefile = (ROOT / required_files[9]).read_text(encoding="utf-8")
roadmap = (ROOT / required_files[10]).read_text(encoding="utf-8")

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
    "TimerIntentRepositoryStatus",
    "TimerIntentRepositoryResult",
    "TimerIntentRepositoryListResult",
    "class TimerIntentRepository",
    "alreadyExists",
    "conflict",
    "findEquivalent",
    "expectedRevision",
]:
    if token not in repository_header:
        raise SystemExit(f"missing TimerIntent repository header marker: {token}")

for token in [
    '"CREATE TABLE IF NOT EXISTS timer_intents ("',
    '"intent_revision INTEGER NOT NULL CHECK(intent_revision > 0),"',
    '"semantic_identity TEXT NOT NULL"',
    '"BEGIN IMMEDIATE TRANSACTION;"',
    '"WHERE timer_intent_id=? AND intent_revision=?;"',
    "timerIntentRevisionMatches(expectedRevision, current.intentRevision)",
    "expectedRevisionNumber + 1",
    "timerIntentTerminal(current.state)",
    "timerIntentCanTransition(current.state, next.state)",
    "timerIntentSemanticIdentity(intent.spec) == columnText(statement, 46)",
]:
    if token not in repository_source:
        raise SystemExit(f"missing TimerIntent repository source marker: {token}")

for token in [
    'database.open(":memory:")',
    'created.intent.intentRevision == "1"',
    "TimerIntentRepositoryStatus::alreadyExists",
    "TimerIntentRepositoryStatus::conflict",
    'conflict.intent.intentRevision == "2"',
    "manufacturedRevision.intentRevision = \"3\"",
    "TimerIntentState::active",
    "TimerIntentState::satisfied",
    "TimerIntentRepositoryStatus::notFound",
    "twoEquivalent.intents.size() == 2",
]:
    if token not in repository_test:
        raise SystemExit(f"missing TimerIntent repository regression marker: {token}")

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
    if token not in contract_doc:
        raise SystemExit(f"missing Phase-64 Slice-1 contract statement: {token}")

for token in [
    "TimerIntentRepository",
    "repository issues the successor revision",
    "stale writer",
    "semantic identity",
    "core/timers/src/*Repository.cpp",
    "TimerAssignment",
    "NativeTimerBinding",
    "no real yaVDR acceptance",
    "mutations=enabled",
]:
    if token not in repository_doc:
        raise SystemExit(f"missing Phase-64 Slice-2 repository statement: {token}")

for token in [
    "test-phase64-timer-intent-contract-architecture",
    "test-phase64-timer-intent-contract:",
    "test-phase64-timer-intent-repository:",
    "core/timers/src/TimerIntent.cpp",
    "core/timers/src/TimerIntentRepository.cpp",
    "core/timers/tests/test_timer_intent_repository.cpp",
    "test-fast: test-phase64-timer-intent-contract test-phase64-timer-intent-repository",
    "test-architecture: test-phase64-timer-intent-contract-architecture",
]:
    if token not in make_fragment:
        raise SystemExit(f"missing Phase-64 test-graph marker: {token}")

if "include mk/phase64-timer-intent-tests.mk" not in makefile:
    raise SystemExit("Phase-64 TimerIntent test fragment is not included by Makefile")

for token in [
    "Phase 63 - Backend Agent and Secure Multi-Site Runtime",
    "Phase 64 - Timer Intent and Multi-Backend Orchestration",
    "Phase 64 Slice 1 — TimerIntent Domain Contract",
    "Phase 64 Slice 2 — TimerIntent Persistence and Repository Semantics",
    "Status: **Completed.**",
    "Status: **Active; Slice 2 is TimerIntent persistence and repository semantics.**",
    "No TimerAssignment; no NativeTimerBinding; no scheduler or failover execution",
]:
    if token not in roadmap:
        raise SystemExit(f"missing Phase-64 roadmap boundary: {token}")

# Slice 2 adds only domain persistence. Production wiring outside core/timers
# would skip the separately reviewed assignment, scheduling and native-mutation
# slices.
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

for relative in [
    required_files[0],
    required_files[1],
    required_files[2],
    required_files[3],
    required_files[4],
    required_files[5],
    required_files[8],
]:
    text = (ROOT / relative).read_text(encoding="utf-8")
    if "mutations=enabled" in text:
        raise SystemExit(f"Phase-64 persistence slice must not enable mutations: {relative}")

print("Phase-64 TimerIntent contract and repository check passed")
print("Slice-2 boundary: persistence only; assignment/native runtime deferred")
