#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "header": ROOT / "core/daemon/include/NativeTimerCreateReadbackReconciliation.h",
    "source": ROOT / "core/daemon/src/NativeTimerCreateReadbackReconciliation.cpp",
    "test": ROOT / "core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "phase69_make": ROOT / "mk/phase69-public-api-tests.mk",
    "ci_groups": ROOT / "mk/test-groups.mk",
    "doc": ROOT / "docs/development/phase-69c-native-timer-create-readback-reconciliation.md",
    "productive_runtime": ROOT / "core/daemon/src/NativeTimerCreateProductiveRuntime.cpp",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            "missing Phase-69.C Timer CREATE readback-reconciliation file: "
            + str(path.relative_to(ROOT)))
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "header": [
        "reconcileNativeTimerCreateReadback(",
        "NativeTimerCreateReadbackReconciliationStatus",
        "MutationOperationRepository& operationRepository",
        "NativeTimerCreateReadbackVerificationService&",
        "TimerAssignmentFulfillmentService& fulfillmentService",
        "NativeTimerCreateOperationCompletionService&",
    ],
    "source": [
        "operationRepository.findPayloadByOperationId(expectation.operationId)",
        "parseNativeTimerCreateOperationPayload(",
        "verificationService.verify(expectation, readbackEvidence)",
        "fulfillmentService.bindVerified(",
        "payload.expectedAssignmentRevision",
        "payload.expectedIntentRevision",
        "verified.binding.bindingRevision",
        "completionService.complete(expectation, reconciledAt)",
    ],
    "test": [
        'reserved.operation.operationRevision == "1"',
        'claimed.operation.operationRevision == "2"',
        'afterOutcome.operation.operationRevision == "3"',
        "NativeTimerCreateReadbackVerificationStatus::staleEvidence",
        "TimerAssignmentState::provisioning",
        "NativeTimerCreateReadbackReconciliationStatus::completed",
        "TimerAssignmentFulfillmentStatus::bound",
        "NativeTimerCreateOperationCompletionStatus::completed",
        'completedOperation.operation.operationRevision == "4"',
        "NativeTimerCreateReadbackReconciliationStatus::alreadyCompleted",
        "NativeTimerCreateReadbackVerificationStatus::alreadyVerified",
        "TimerAssignmentFulfillmentStatus::alreadyBound",
    ],
    "daemon_sources": [
        "core/daemon/src/NativeTimerCreateReadbackReconciliation.cpp",
    ],
    "phase69_make": [
        "test-phase69-native-timer-create-readback-reconciliation",
        "tools/check_phase69_native_timer_create_readback_reconciliation.py",
        "core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp",
    ],
    "ci_groups": [
        "test-phase69-native-timer-create-readback-reconciliation",
    ],
    "doc": [
        "NativeTimerCreateReadbackExpectation",
        "NativeTimerCreateReadbackVerificationService::verify()",
        "TimerAssignmentFulfillmentService::bindVerified()",
        "NativeTimerCreateOperationCompletionService::complete()",
        "NATIVE_EFFECT_REACHABLE=NO",
        "YAVDR_ACCEPTANCE_REQUIRED=NO",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Timer CREATE readback-reconciliation marker in {label}: {token}")

source = contents["source"]
header = contents["header"]

for forbidden in [
    "CREATE TABLE",
    "ALTER TABLE",
    "Repository repository_",
    "class NativeTimerCreateReadbackReconciliationService",
]:
    if forbidden in source or forbidden in header:
        raise SystemExit(
            "readback reconciliation must not introduce a new state authority: "
            + forbidden)

for forbidden_call in [
    "BackendAgentNativeTimerCreateReservationService",
    "claimAfterReservation(",
    "activateDispatching(",
    "applyOutcome(",
    "BackendAgentCommandRepository",
    "SuiteBridge",
]:
    if forbidden_call in source:
        raise SystemExit(
            "readback reconciliation must not open dispatch/native execution: "
            + forbidden_call)

adapter_call = "reconcileNativeTimerCreateReadback("
for scan_root in [
    ROOT / "api",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "security",
]:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc"
        }:
            continue
        if path in {paths["header"], paths["source"], paths["test"], paths["productive_runtime"]}:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if adapter_call in text:
            raise SystemExit(
                "Timer CREATE readback reconciliation must remain productively "
                "uninvoked: " + str(path.relative_to(ROOT)))

print("Phase-69.C native Timer CREATE readback-reconciliation check passed")
print(
    "Boundary: accepted outcome expectation can reach verified binding and "
    "operation completion, but Agent pollability/native CREATE remain dormant")
