#!/usr/bin/env python3
"""Architecture guard for Phase 64 Slice 23 native Timer delete dispatch contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerDeleteDispatchService.h"
SOURCE = ROOT / "core/timers/src/NativeTimerDeleteDispatchService.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_delete_dispatch_service.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-delete-dispatch.md"
FRAGMENT = ROOT / "mk/phase64-native-timer-delete-dispatch-tests.mk"
MAKEFILE = ROOT / "Makefile"

failures: list[str] = []

def require(path: Path, markers: list[str]) -> None:
    if not path.is_file():
        failures.append(f"missing Slice-23 file: {path.relative_to(ROOT)}")
        return
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses required marker: {marker}")

require(HEADER, [
    "NativeTimerDeleteDispatchService",
    "NativeTimerDeleteDispatchClaim",
    "expectedNativeTimerFingerprint",
    "NativeTimerDeleteExecutorOutcome",
    "rejectedWithoutEffect",
    "acceptedUnverified",
    "outcomeUnknown",
    "dispatchStartedAt",
    "NativeTimerAbsenceReadbackExpectation",
])
require(SOURCE, [
    "MutationOperationState::dispatching",
    "MutationOperationState::failedVerified",
    "MutationOperationState::executedUnverified",
    "MutationOperationState::outcomeUnknown",
    "operationRepository_.transition",
    "operation.expectedResourceFingerprint ==",
    "handoff.expectedNativeTimerFingerprint",
    "claim.expectedNativeTimerFingerprint =",
    "claim.expectedNativeTimerFingerprint.empty()",
    "binding.bindingRevision == handoff.expectedBindingRevision",
    "binding.observedFingerprint != handoff.expectedNativeTimerFingerprint",
    "binding.missingSince != 0",
    "binding.driftState != NativeTimerBindingDriftState::none",
    "expectation.readbackNotBefore = outcome.dispatchStartedAt",
    "nativeTimerAbsenceReadbackExpectationValid",
])
require(TEST, [
    "alreadyClaimed",
    "acceptedUnverified",
    "outcomeUnknown",
    "failedVerified",
    "readbackNotBefore",
    "bindingRevisionConflict",
    "binding:fingerprint-race",
    "mutateObservedState",
    "NativeTimerDeleteDispatchClaimStatus::bindingStateConflict",
    "fingerprintOperation.operation.state == MutationOperationState::accepted",
    "tamperedHandoff.expectedNativeTimerFingerprint",
    "NativeTimerDeleteDispatchClaimStatus::identityConflict",
])
require(DOC, [
    "Phase 64 Slice 23",
    "PR #175",
    "accepted -> dispatching",
    "failed_verified",
    "executed_unverified",
    "outcome_unknown",
    "readbackNotBefore",
    "no Agent/VDR transport wiring",
    "Slice 24",
])
require(FRAGMENT, [
    "test-phase64-native-timer-delete-dispatch",
    "NativeTimerDeleteDispatchService.cpp",
    "test_native_timer_delete_dispatch_service.cpp",
    "test-fast: test-phase64-native-timer-delete-dispatch",
])
require(MAKEFILE, ["include mk/phase64-native-timer-delete-dispatch-tests.mk"])

source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
for forbidden in [
    "BackendAgentCommand",
    "BackendAgentCommandClient",
    "SuiteBridge",
    "SVDRP",
    "RESTfulAPI",
    "sqlite3_",
    "TimerAssignmentRepository",
    "DaemonRuntime",
]:
    if forbidden in source:
        failures.append(f"Slice-23 dispatch contract contains forbidden runtime coupling: {forbidden}")

for source_manifest in (ROOT / "mk").glob("*-sources.mk"):
    text = source_manifest.read_text(encoding="utf-8")
    if "NativeTimerDeleteDispatchService.cpp" in text:
        failures.append(
            f"Slice-23 dispatch service is unexpectedly wired into runtime sources: {source_manifest.relative_to(ROOT)}"
        )

if failures:
    print("Phase-64 native Timer delete dispatch architecture check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 native Timer delete dispatch architecture check passed")
