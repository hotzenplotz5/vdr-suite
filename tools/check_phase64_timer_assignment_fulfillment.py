#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

files = [
    "core/timers/include/TimerAssignmentFulfillmentService.h",
    "core/timers/src/TimerAssignmentFulfillmentService.cpp",
    "core/timers/tests/test_timer_assignment_fulfillment_service.cpp",
    "docs/development/phase-64-timer-assignment-fulfillment.md",
    "mk/phase64-timer-assignment-fulfillment-tests.mk",
    "Makefile",
]
for relative in files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing TimerAssignment fulfillment file: {relative}")

header = (ROOT / files[0]).read_text(encoding="utf-8")
source = (ROOT / files[1]).read_text(encoding="utf-8")
test = (ROOT / files[2]).read_text(encoding="utf-8")
doc = (ROOT / files[3]).read_text(encoding="utf-8")
mk = (ROOT / files[4]).read_text(encoding="utf-8")
makefile = (ROOT / files[5]).read_text(encoding="utf-8")

for token in [
    "provisioningStarted", "alreadyProvisioning", "alreadyBound",
    "assignmentRevisionConflict", "bindingRevisionConflict",
    "class TimerAssignmentFulfillmentService",
]:
    if token not in header:
        raise SystemExit(f"missing fulfillment header marker: {token}")

for token in [
    "current.state =",  # prohibited below if accidental direct mutation
]:
    if token in source:
        raise SystemExit("fulfillment must not mutate repository snapshots in place")

for token in [
    "assignmentRepository_.findById(timerAssignmentId)",
    "next.state = TimerAssignmentState::provisioning",
    "assignmentRepository_.update(",
    "bindingRepository_.findById(nativeTimerBindingId)",
    "binding.ownership != NativeTimerBindingOwnership::managed",
    "!binding.lastVerifiedOperationId.empty()",
    "next.state = TimerAssignmentState::bound",
    "next.nativeTimerBindingId = nativeTimerBindingId",
]:
    if token not in source:
        raise SystemExit(f"missing fulfillment source marker: {token}")

for forbidden in ["SuiteBridge", "BackendAgentCommand", "SVDRP", "RestfulApi", "MutationOperationRepository"]:
    if forbidden in header + source:
        raise SystemExit(f"premature fulfillment boundary crossing: {forbidden}")

for token in [
    "provisioningStarted", "alreadyProvisioning",
    "assignmentRevisionConflict", "TimerAssignmentFulfillmentStatus::bound",
    "alreadyBound", "generationConflict", "bindingNotFound",
    "test_timer_assignment_fulfillment_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing fulfillment regression marker: {token}")

for token in [
    "selected -> provisioning",
    "provisioning -> bound",
    "verified managed NativeTimerBinding",
    "revision-fenced",
    "outcome_unknown",
]:
    if token not in doc:
        raise SystemExit(f"missing fulfillment documentation marker: {token}")

if "include mk/phase64-timer-assignment-fulfillment-tests.mk" not in makefile:
    raise SystemExit("fulfillment make fragment is not included")

for token in [
    "test-phase64-timer-assignment-fulfillment-architecture",
    "test-phase64-timer-assignment-fulfillment:",
    "test-fast: test-phase64-timer-assignment-fulfillment",
    "test-architecture: test-phase64-timer-assignment-fulfillment-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing fulfillment make marker: {token}")

print("Phase-64 TimerAssignment fulfillment check passed")
