#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "command_h": ROOT / "core/agent/include/BackendAgentCommand.h",
    "command_cpp": ROOT / "core/agent/src/BackendAgentCommand.cpp",
    "command_json": ROOT / "core/agent/src/BackendAgentCommandJson.cpp",
    "delivery_h": ROOT / "core/agent/include/BackendAgentCommandDelivery.h",
    "delivery_cpp": ROOT / "core/agent/src/BackendAgentCommandDelivery.cpp",
    "create_h": ROOT / "core/agent/include/BackendAgentNativeTimerCreate.h",
    "create_cpp": ROOT / "core/agent/src/BackendAgentNativeTimerCreate.cpp",
    "create_handler": ROOT / "core/agent/src/BackendAgentNativeTimerCreateCommandHandler.cpp",
    "reconciliation_guard": ROOT / "tools/check_phase69_native_timer_create_reconciliation_runtime.py",
    "doc": ROOT / "docs/development/phase-69c-native-timer-create-outcome-evidence.md",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C Timer CREATE outcome-evidence file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "command_h": [
        "std::string resultEvidence;",
    ],
    "command_cpp": [
        "appendField(canonical, result.resultEvidence);",
        "backendAgentCommandSafeText(value.resultEvidence, 4096)",
    ],
    "command_json": [
        '"resultEvidence"',
        "result.resultEvidence=",
        "r.resultEvidence",
    ],
    "delivery_h": [
        "std::optional<BackendAgentCommandResult> resultForCommand(",
    ],
    "delivery_cpp": [
        "result_evidence TEXT NOT NULL DEFAULT ''",
        "result.resultEvidence",
        "BackendAgentCommandRepository::resultForCommand(",
    ],
    "create_h": [
        "backendAgentNativeTimerCreateResultEvidence(",
        "backendAgentNativeTimerCreateParseResultEvidence(",
    ],
    "create_cpp": [
        "native-timer-create-result-evidence/1|",
        "backendAgentNativeTimerCreateEvidenceMatches(",
    ],
    "create_handler": [
        "result.resultEvidence =",
        "backendAgentNativeTimerCreateResultEvidence(",
    ],
    "doc": [
        "BackendAgentCommandResult",
        "dispatchStartedAt",
        "evidenceReference",
        "no Agent command becomes pollable",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Timer CREATE outcome-evidence marker in {label}: {token}")

# Keep the generic result authority backward compatible for commands that do not
# carry typed evidence. The new field is optional on the wire, but once present
# it is part of the durable result identity.
json_text = contents["command_json"]
if "legacyResultKeys" not in json_text or "extendedResultKeys" not in json_text:
    raise SystemExit(
        "Agent result JSON must explicitly accept both legacy and evidence-bearing v1 shapes")

# The evidence must stay in the existing command-result authority. This slice
# must not create a parallel result/evidence table or a Timer-specific lifecycle.
delivery = contents["delivery_cpp"]
for forbidden in [
    "backend_agent_native_timer_create_results",
    "backend_agent_native_timer_create_evidence",
]:
    if forbidden in delivery:
        raise SystemExit(
            f"parallel Timer CREATE result authority is forbidden: {forbidden}")

# This prerequisite still does not activate the native effect. The accepted
# Phase-69.C reconciliation guard must continue closing the productive calls.
reconciliation = contents["reconciliation_guard"]
for forbidden_call in [
    "backendAgentNativeTimerCreateReservationService_->reserve(",
    "nativeTimerCreateDispatchService_->claimAfterReservation(",
    "backendAgentNativeTimerCreateActivationService_->activateDispatching(",
    "nativeTimerCreateDispatchService_->applyOutcome(",
    "nativeTimerCreateReadbackVerificationService_->verify(",
    "timerAssignmentFulfillmentService_->bindVerified(",
    "nativeTimerCreateOperationCompletionService_->complete(",
]:
    if forbidden_call not in reconciliation:
        raise SystemExit(
            "accepted dormant-boundary guard disappeared: " + forbidden_call)

print("Phase-69.C native Timer CREATE outcome-evidence check passed")
print("Boundary: typed executor evidence is durable in the existing Agent result authority; native dispatch remains dormant")
