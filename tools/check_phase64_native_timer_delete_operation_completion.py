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

# Phase-64's dormant source guard is superseded only for this exact
# Phase-69 daemon successor, which uses the already-audited bounded
# Agent assignment, durable typed result and complete native inventory.
# No other source manifest may gain this dependency.
delete_runtime_path = ROOT / "core/daemon/src/NativeTimerDeleteProductiveRuntime.cpp"
delete_outcome_path = ROOT / "core/daemon/src/NativeTimerDeleteResultOutcomeApplication.cpp"
delete_runtime = delete_runtime_path.read_text(encoding="utf-8") if delete_runtime_path.is_file() else ""
delete_outcome = delete_outcome_path.read_text(encoding="utf-8") if delete_outcome_path.is_file() else ""
delete_successor_valid = all(marker in delete_runtime for marker in (
    "publicNativeTimerDeletePreflight(",
    "applyDurableNativeTimerDeleteResult(",
    "verification.verify(",
    "completion.complete(",
    "MutationOperationState::outcomeUnknown",
    "BackendAgentNativeTimerDeleteAssignmentService& assigner",
)) and all(marker in delete_outcome for marker in (
    "backendAgentNativeTimerDeleteParseResultEvidence(",
    "dispatchService.applyOutcome(",
    "NativeTimerBindingOwnership::managed",
    "NativeTimerBindingOwnership::adopted",
))
required_runtime_sources = (
    "NativeTimerDeleteOperationPreparationService.cpp",
    "NativeTimerDeleteDispatchService.cpp",
    "NativeTimerDeleteOperationCompletionService.cpp",
    "NativeTimerDeleteProductiveRuntime.cpp",
    "NativeTimerDeleteResultOutcomeApplication.cpp",
)

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "NativeTimerDeleteOperationCompletionService.cpp" in text:
        if (
            source_manifest.name == "daemon-sources.mk"
            and delete_successor_valid
            and all(src in text for src in required_runtime_sources)
        ):
            continue
        failures.append(
            f"Slice-21 completion service wired outside verified DELETE daemon successor: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete operation completion architecture check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete operation completion architecture check passed")
