#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/TimerAssignmentSchedulingService.h",
    "core/timers/src/TimerAssignmentSchedulingService.cpp",
    "core/timers/tests/test_timer_assignment_replica_scheduling_service.cpp",
    "docs/development/phase-64-timer-assignment-replica-scheduling-handoff.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 replica scheduling file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")

for token in [
    "TimerAssignmentReplicaSchedulingRequest",
    "scheduleReplica",
    "replicaTargetSatisfied",
    "assignmentSetConflict",
]:
    if token not in header:
        raise SystemExit(f"missing replica scheduling header marker: {token}")

start = source.find("TimerAssignmentSchedulingService::scheduleReplica")
if start < 0:
    raise SystemExit("missing scheduleReplica implementation")
end = source.find("const char* timerAssignmentSchedulingStatusName", start)
replica_source = source[start:] if end < 0 else source[start:end]

for token in [
    "intentRepository_.findById",
    "assignmentRepository_.assignmentSetRevisionForIntent",
    "assignmentRepository_.listForIntent",
    "TimerAssignmentRole::replica",
    "planTimerAssignment(planning)",
    '"replica_target_satisfied"',
    "TimerAssignmentSchedulingStatus::replicaTargetSatisfied",
    "assignmentRepository_.createAgainstAssignmentSetRevision",
    "setRevision.assignmentSetRevision",
    "TimerAssignmentSchedulingStatus::assignmentSetConflict",
]:
    if token not in replica_source and token not in source:
        raise SystemExit(f"missing replica scheduling source marker: {token}")

set_pos = replica_source.find("assignmentSetRevisionForIntent")
list_pos = replica_source.find("listForIntent")
plan_pos = replica_source.find("planTimerAssignment")
create_pos = replica_source.find("createAgainstAssignmentSetRevision")
if min(set_pos, list_pos, plan_pos, create_pos) < 0:
    raise SystemExit("incomplete replica scheduling ordering markers")
if not set_pos < list_pos < plan_pos < create_pos:
    raise SystemExit(
        "replica scheduling must read set revision before assignments, then plan, then fenced create")
if "assignmentRepository_.create(candidate)" in replica_source:
    raise SystemExit("replica scheduling bypasses assignment-set revision fence")

for token in [
    "TimerAssignmentSchedulingStatus::persisted",
    "TimerAssignmentSchedulingStatus::alreadyPersisted",
    "TimerAssignmentSchedulingStatus::replicaTargetSatisfied",
    "TimerAssignmentRole::replica",
    'replica.assignment.backendId == "backend:beta"',
    'afterSatisfiedList.assignments.size() == 2',
    'afterSatisfiedSet.assignmentSetRevision == "2"',
    '"replica_target_satisfied"',
    "TimerAssignmentState::unassigned",
    "TimerAssignmentSchedulingStatus::intentRevisionConflict",
    "TimerAssignmentSchedulingStatus::planningInvalid",
]:
    if token not in test:
        raise SystemExit(f"missing replica scheduling regression marker: {token}")

for token in [
    "Replica TimerAssignment Scheduling Handoff",
    "assignmentSetRevision",
    "read before the assignment list",
    "createAgainstAssignmentSetRevision",
    "replica_target_satisfied",
    "successful no-op",
    "assignment_set_conflict",
    "must re-plan",
    "Replacement remains deferred",
    "NativeTimerBinding",
    "no installed runtime path",
]:
    if token not in doc:
        raise SystemExit(f"missing Phase-64 Slice-8 statement: {token}")

for token in [
    "test-phase64-timer-assignment-replica-scheduling-architecture",
    "test-phase64-timer-assignment-replica-scheduling:",
    "core/timers/src/TimerAssignmentSetRevisionRepository.cpp",
    "core/timers/src/TimerAssignmentSchedulingService.cpp",
    "core/timers/tests/test_timer_assignment_replica_scheduling_service.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(f"missing replica scheduling test-graph marker: {token}")

service_text = header + source + test
for forbidden in [
    "#include <sqlite3.h>",
    "sqlite3_",
    "scheduleReplacement",
    "TimerAssignmentRole::replacement",
    "NativeTimerBindingRepository",
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in service_text:
        raise SystemExit(f"premature replica scheduling boundary crossing: {forbidden}")

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
        if "TimerAssignmentReplicaSchedulingRequest" in text or "scheduleReplica(" in text:
            raise SystemExit(
                "premature replica scheduling runtime wiring: "
                + str(path.relative_to(ROOT)))

print("Phase-64 TimerAssignment replica scheduling handoff check passed")
print(
    "Slice-8 boundary: set-revision-fenced replica persistence only; "
    "replacement/native runtime deferred")
