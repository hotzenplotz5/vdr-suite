#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
required = [
    "core/timers/include/TimerAssignmentReassignmentService.h",
    "core/timers/src/TimerAssignmentReassignmentService.cpp",
    "core/timers/src/TimerAssignmentReassignmentRepository.cpp",
    "core/timers/tests/test_timer_assignment_reassignment_service.cpp",
    "docs/development/phase-64-timer-reassignment-failover-contract.md",
    "mk/phase64-timer-reassignment-failover-tests.mk",
]
for relative in required:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Phase-64 reassignment file: {relative}")

service = (ROOT / required[1]).read_text(encoding="utf-8")
repository = (ROOT / required[2]).read_text(encoding="utf-8")
test = (ROOT / required[3]).read_text(encoding="utf-8")
doc = (ROOT / required[4]).read_text(encoding="utf-8")

for token in [
    "assignmentSetRevisionForIntent",
    "listForIntent",
    "TimerAssignmentRole::replacement",
    "planTimerAssignment(planning)",
    "createControlledReplacement",
    "MutationOperationState::succeeded",
    'operation.operation.actionFamily != "timer.delete"',
    "NativeTimerBindingDriftState::expectedTransition",
    "binding.binding.missingSince == 0",
    "binding.binding.observedState.recording",
]:
    if token not in service:
        raise SystemExit(f"missing reassignment service fence: {token}")

set_pos = service.find("assignmentSetRevisionForIntent")
list_pos = service.find("listForIntent", set_pos)
plan_pos = service.find("planTimerAssignment", list_pos)
persist_pos = service.find("createControlledReplacement", plan_pos)
if min(set_pos, list_pos, plan_pos, persist_pos) < 0 or not (
    set_pos < list_pos < plan_pos < persist_pos
):
    raise SystemExit("reassignment order must be set revision, list, plan, atomic persist")

for token in [
    "CREATE TEMP TRIGGER trg_timer_assignment_reassignment_before",
    "assignment_set_conflict",
    "old_owner_conflict",
    "state='superseded'",
    "CREATE TEMP TRIGGER trg_timer_assignment_reassignment_after",
    "INSERT INTO timer_assignment_reassignments",
    "NEW.assignment_epoch",
]:
    if token not in repository:
        raise SystemExit(f"missing atomic reassignment repository fence: {token}")

for forbidden in [
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "Phase 65",
]:
    if forbidden in service + repository + test:
        raise SystemExit(f"reassignment crosses runtime boundary: {forbidden}")

for token in [
    "before dispatch",
    "verified absent",
    "outcome_unknown",
    "atomically supersede",
    "public SuiteBridge SVDRP help remains closed",
]:
    if token not in doc:
        raise SystemExit(f"missing documented reassignment contract: {token}")

print("Phase-64 Timer reassignment/failover architecture check passed")
