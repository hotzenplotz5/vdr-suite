#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreateActivation.h": [
        "BackendAgentNativeTimerCreateActivationService",
        "activateDispatching",
        "providerSelectionStale",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateActivation.cpp": [
        "operationRepository_.findById(operationId)",
        "MutationOperationState::dispatching",
        "parseNativeTimerCreateCommandReservationReference",
        "reservationRepository_.findByCommandId",
        "nextRevision(agentPayload.operationRevision, operation.operationRevision)",
        "findPayloadByOperationId(operationId)",
        "findAssignmentForOperation",
        "BackendAgentCommandReservationActivationService activation",
        "activation.activate(dispatchReference.commandId)",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_activation.cpp": [
        "cannot be activated before",
        "no new command/job/attempt identity is generated",
        "provider replacement after dispatching",
        "only legal command identity",
    ],
}

errors = []
for relative, markers in required.items():
    path = ROOT / relative
    if not path.exists():
        errors.append(f"missing {relative}")
        continue
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            errors.append(f"{relative}: missing marker {marker}")

source = (ROOT / "core/agent/src/BackendAgentNativeTimerCreateActivation.cpp").read_text(
    encoding="utf-8"
)
for forbidden in (
    "backendAgentGenerateOpaqueId",
    "reservationRepository_.reserve",
    "reserveWithPayload",
    "MutationOperationState::accepted,",
):
    if forbidden in source:
        errors.append(f"CREATE activation must not create a new dispatch identity/state: {forbidden}")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-activation-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE activation tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE activation architecture guard passed")
