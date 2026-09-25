#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "header": ROOT / "core/daemon/include/NativeTimerCreateResultOutcomeApplication.h",
    "source": ROOT / "core/daemon/src/NativeTimerCreateResultOutcomeApplication.cpp",
    "test": ROOT / "core/daemon/tests/test_native_timer_create_result_outcome_application.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "phase69_make": ROOT / "mk/phase69-public-api-tests.mk",
    "ci_groups": ROOT / "mk/test-groups.mk",
    "doc": ROOT / "docs/development/phase-69c-native-timer-create-outcome-application.md",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C Timer CREATE outcome-application file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "header": [
        "applyDurableNativeTimerCreateResult(",
        "NativeTimerCreateResultOutcomeApplicationStatus",
        "MutationOperationRepository& operationRepository",
        "NativeTimerCreateDispatchService& dispatchService",
    ],
    "source": [
        "commandRepository.findAssignment(commandId)",
        "commandRepository.resultForCommand(commandId)",
        "backendAgentNativeTimerCreateParseResultEvidence(",
        "operationRepository.findById(evidence.operationId)",
        "revisionAdvancedBy(",
        "evidence.operationRevision, operation.operationRevision, 1",
        "outcome.operationRevision = operation.operationRevision;",
        "outcome.dispatchStartedAt = evidence.dispatchStartedAt;",
        "outcome.completedAt = evidence.completedAt;",
        "outcome.evidenceReference = evidence.evidenceReference;",
        "dispatchService.applyOutcome(outcome)",
    ],
    "test": [
        'reservedOperation.operation.operationRevision == "1"',
        'claimed.operation.operationRevision == "2"',
        'parsedEvidence.operationRevision == "1"',
        "NativeTimerCreateResultOutcomeApplicationStatus::applied",
        "NativeTimerCreateResultOutcomeApplicationStatus::alreadyApplied",
        "applied.dispatch.expectation.readbackNotBefore == 124",
        'afterApplication.operation.operationRevision == "3"',
    ],
    "daemon_sources": [
        "core/daemon/src/NativeTimerCreateResultOutcomeApplication.cpp",
    ],
    "phase69_make": [
        "test-phase69-native-timer-create-outcome-application",
        "tools/check_phase69_native_timer_create_outcome_application.py",
        "core/daemon/tests/test_native_timer_create_result_outcome_application.cpp",
    ],
    "ci_groups": [
        "test-phase69-native-timer-create-outcome-application",
    ],
    "doc": [
        "reservation revision",
        "dispatch revision",
        "resultForCommand()",
        "applyOutcome()",
        "no Agent command becomes pollable",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Timer CREATE outcome-application marker in {label}: {token}")

source = contents["source"]
for forbidden in [
    "CREATE TABLE",
    "ALTER TABLE",
    "class NativeTimerCreateResultOutcomeApplicationService",
    "Repository repository_",
]:
    if forbidden in source or forbidden in contents["header"]:
        raise SystemExit(
            f"outcome application must not introduce a new state authority: {forbidden}")

for forbidden in [
    "outcome.dispatchStartedAt = operation.",
    "outcome.dispatchStartedAt = durableResult",
    "outcome.evidenceReference = operation.",
    "outcome.evidenceReference = durableResult",
]:
    if forbidden in source:
        raise SystemExit(
            f"typed Agent evidence must remain authoritative: {forbidden}")

for forbidden_call in [
    "backendAgentNativeTimerCreateReservationService_->reserve(",
    "nativeTimerCreateDispatchService_->claimAfterReservation(",
    "backendAgentNativeTimerCreateActivationService_->activateDispatching(",
    "nativeTimerCreateReadbackVerificationService_->verify(",
    "timerAssignmentFulfillmentService_->bindVerified(",
    "nativeTimerCreateOperationCompletionService_->complete(",
]:
    if forbidden_call in source:
        raise SystemExit(
            "outcome application must not open a later Timer CREATE boundary: "
            + forbidden_call)

adapter_call = "applyDurableNativeTimerCreateResult("
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
        if path in {paths["header"], paths["source"], paths["test"]}:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if adapter_call in text:
            raise SystemExit(
                "Timer CREATE outcome adapter must remain productively uninvoked: "
                + str(path.relative_to(ROOT)))

print("Phase-69.C native Timer CREATE outcome-application check passed")
print("Boundary: durable typed result can be applied to MutationOperation, but reservation/activation/readback remain dormant")
