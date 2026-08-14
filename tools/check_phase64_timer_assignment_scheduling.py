#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerAssignmentSchedulingService.h",
    "core/timers/src/TimerAssignmentSchedulingService.cpp",
    "core/timers/tests/test_timer_assignment_scheduling_service.cpp",
    "docs/development/phase-64-timer-assignment-scheduling-handoff.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 TimerAssignment scheduling file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")

for token in [
    "TimerAssignmentSchedulingStatus",
    "TimerAssignmentPrimarySchedulingRequest",
    "TimerAssignmentSchedulingResult",
    "TimerAssignmentSchedulingService",
    "schedulePrimary",
    "expectedIntentRevision",
]:
    if token not in header:
        raise SystemExit(
            f"missing TimerAssignment scheduling header marker: {token}")

for token in [
    "intentRepository_.findById",
    "assignmentRepository_.listForIntent",
    "planTimerAssignment(planning)",
    "TimerAssignmentState::selected",
    "TimerAssignmentState::unassigned",
    "assignmentRepository_.create(candidate)",
    "TimerAssignmentRepositoryStatus::ownershipConflict",
    '"active_primary_exists"',
    "alreadyPersisted",
]:
    if token not in source:
        raise SystemExit(
            f"missing TimerAssignment scheduling source marker: {token}")

for token in [
    "TimerAssignmentSchedulingStatus::persisted",
    "TimerAssignmentSchedulingStatus::alreadyPersisted",
    "TimerAssignmentSchedulingStatus::activePrimaryExists",
    "TimerAssignmentSchedulingStatus::intentRevisionConflict",
    "TimerAssignmentSchedulingStatus::planningInvalid",
    "TimerAssignmentState::selected",
    "TimerAssignmentState::unassigned",
    'assignmentRepository.findById("assignment:stale")',
]:
    if token not in test:
        raise SystemExit(
            f"missing TimerAssignment scheduling regression marker: {token}")

for token in [
    "Primary TimerAssignment Scheduling Handoff",
    "exact current TimerIntent",
    "active ownership assignments",
    "repository remains the sole issuer",
    "second active primary",
    "idempotent replay",
    "Replica and replacement",
    "NativeTimerBinding",
    "no installed runtime path",
]:
    if token not in doc:
        raise SystemExit(
            f"missing Phase-64 Slice-6 scheduling statement: {token}")

for token in [
    "test-phase64-timer-assignment-scheduling-architecture",
    "test-phase64-timer-assignment-scheduling:",
    "core/timers/src/TimerAssignmentSchedulingService.cpp",
    "core/timers/tests/test_timer_assignment_scheduling_service.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(
            f"missing TimerAssignment scheduling test-graph marker: {token}")

slice_text = header + source + test
for forbidden in [
    "#include <sqlite3.h>",
    "sqlite3_",
    "NativeTimerBindingRepository",
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in slice_text:
        raise SystemExit(
            f"premature TimerAssignment scheduling boundary crossing: {forbidden}")

for forbidden in [
    "scheduleReplica",
    "scheduleReplacement",
    "TimerAssignmentRole::replica",
    "TimerAssignmentRole::replacement",
]:
    if forbidden in header + source:
        raise SystemExit(
            f"Slice-6 primary-only boundary widened prematurely: {forbidden}")

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
text_suffixes = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk",
    ".conf", ".service",
}
for root in forbidden_roots:
    if not root.exists():
        continue
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "TimerAssignmentSchedulingService" in text:
            raise SystemExit(
                "premature TimerAssignment scheduling runtime wiring: "
                + str(path.relative_to(ROOT)))

print("Phase-64 TimerAssignment scheduling handoff check passed")
print(
    "Slice-6 boundary: durable primary assignment handoff only; "
    "replica/replacement/native runtime deferred")
