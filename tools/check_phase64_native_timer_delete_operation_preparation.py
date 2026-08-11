#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 22 delete operation preparation."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerDeleteOperationPreparationService.h"
SOURCE = ROOT / "core/timers/src/NativeTimerDeleteOperationPreparationService.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_delete_operation_preparation_service.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-operation-preparation.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-operation-preparation-tests.mk"
MAKEFILE = ROOT / "Makefile"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-22 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


require(
    HEADER,
    [
        "NativeTimerDeleteOperationPreparationService",
        "NativeTimerDeleteOperationPreparationRequest",
        "NativeTimerDeleteDispatchHandoff",
        "expectedBindingRevision",
        "expectedBackendGeneration",
        "operationRevision",
        "alreadyPrepared",
        "operationStateConflict",
    ],
)
require(
    SOURCE,
    [
        'operation.resourceType = "NativeTimerBinding"',
        'operation.actionFamily = "timer.delete"',
        "MutationOperationVerificationPolicy::readbackRequired",
        "MutationOperationState::accepted",
        "binding.missingSince != 0",
        "binding.driftState != NativeTimerBindingDriftState::none",
        "request.expectedBackendGeneration",
        "nativeTimerBindingRevisionMatches",
        "operationRepository_.reserve",
        "reserved.operation.state != MutationOperationState::accepted",
    ],
)
require(
    TEST,
    [
        "prepared",
        "alreadyPrepared",
        "operationStateConflict",
        "idempotencyConflict",
        "operationConflict",
        "bindingRevisionConflict",
        "generationConflict",
        "ownershipConflict",
        "bindingMissing",
        "driftConflict",
        "bindingNotFound",
    ],
)
require(
    DOC,
    [
        "Phase 64 Slice 22",
        "PR #174",
        "Pre-dispatch only",
        "No premature readback fence",
        "NativeTimerDeleteDispatchHandoff",
        "accepted",
        "no native Timer mutation",
        "Slice 23",
    ],
)
require(
    FRAGMENT,
    [
        "test-phase64-native-timer-delete-operation-preparation",
        "MutationOperationRepository.cpp",
        "NativeTimerBindingRepository.cpp",
        "NativeTimerDeleteOperationPreparationService.cpp",
        "test_native_timer_delete_operation_preparation_service.cpp",
        "test-fast: test-phase64-native-timer-delete-operation-preparation",
    ],
)
require(
    MAKEFILE,
    ["include mk/phase64-native-timer-delete-operation-preparation-tests.mk"],
)

source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
header = HEADER.read_text(encoding="utf-8") if HEADER.is_file() else ""
production = header + "\n" + source

# Explanatory comments may name later-slice concepts. Guard actual code-shape
# coupling/facts rather than rejecting documentation comments.
for forbidden in [
    '#include "NativeTimerAbsenceReadbackExpectation.h"',
    "std::int64_t readbackNotBefore",
    "NativeTimerReadbackOperationState",
    "MutationOperationState::executedUnverified",
    "MutationOperationState::outcomeUnknown",
]:
    if forbidden in production:
        failures.append(f"Slice-22 pre-dispatch service contains forbidden post-dispatch fact: {forbidden}")

for forbidden in [
    "sqlite3_",
    "RESTfulAPI",
    "SuiteBridge",
    "SVDRP",
    "BackendAgentCommand",
    "TimerAssignmentRepository",
    "DaemonRuntime",
]:
    if forbidden in source:
        failures.append(f"Slice-22 pre-dispatch service contains forbidden runtime coupling: {forbidden}")

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "NativeTimerDeleteOperationPreparationService.cpp" in text:
        failures.append(
            f"Slice-22 preparation service is unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete operation preparation architecture check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete operation preparation architecture check passed")
