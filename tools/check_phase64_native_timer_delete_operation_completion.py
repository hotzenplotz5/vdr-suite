#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 21 delete readback operation completion."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerDeleteOperationCompletionService.h"
SOURCE = ROOT / "core/timers/src/NativeTimerDeleteOperationCompletionService.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_delete_operation_completion_service.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-operation-completion.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-operation-completion-tests.mk"
MAKEFILE = ROOT / "Makefile"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-21 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")


require(
    HEADER,
    [
        "NativeTimerDeleteOperationCompletionService",
        "MutationOperationRepository",
        "NativeTimerBindingRepository",
        "NativeTimerAbsenceReadbackExpectation",
        "alreadyCompleted",
        "verificationEvidenceMissing",
        "operationRevisionConflict",
    ],
)
require(
    SOURCE,
    [
        'operation.resourceType == "NativeTimerBinding"',
        'operation.actionFamily == "timer.delete"',
        "operation.expectedRevision == expectation.expectedBindingRevision",
        "MutationOperationVerificationPolicy::readbackRequired",
        "binding.lastVerifiedOperationId == expectation.operationId",
        "binding.lastObservedAt >= expectation.readbackNotBefore",
        "MutationOperationState::succeeded",
        "operation.operationRevision",
        "operationRepository_.transition",
        "NativeTimerBindingOwnership::managed",
        "NativeTimerBindingOwnership::adopted",
    ],
)
require(
    TEST,
    [
        "executedUnverified",
        "outcomeUnknown",
        "alreadyCompleted",
        "identityConflict",
        "verificationPolicyConflict",
        "operationStateConflict",
        "verificationEvidenceMissing",
    ],
)
require(
    DOC,
    [
        "Phase 64 Slice 21",
        "PR #173",
        "Single lifecycle authority",
        "lastVerifiedOperationId",
        "readback_required",
        "no native Timer mutation",
        "Slice 22",
    ],
)
require(
    FRAGMENT,
    [
        "test-phase64-native-timer-delete-operation-completion",
        "MutationOperationRepository.cpp",
        "NativeTimerBindingRepository.cpp",
        "NativeTimerDeleteOperationCompletionService.cpp",
        "test_native_timer_delete_operation_completion_service.cpp",
        "test-fast: test-phase64-native-timer-delete-operation-completion",
    ],
)
require(
    MAKEFILE,
    ["include mk/phase64-native-timer-delete-operation-completion-tests.mk"],
)

source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
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
        failures.append(f"Slice-21 completion service contains forbidden coupling: {forbidden}")

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "NativeTimerDeleteOperationCompletionService.cpp" in text:
        failures.append(
            f"Slice-21 completion service is unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete operation completion architecture check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete operation completion architecture check passed")
