#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/src/BackendAgentCommandDelivery.cpp": [
        'BackendAgentNativeTimerCreatePayload.h',
        'const bool timerCreate=commandType==kBackendAgentNativeTimerCreateCommandType;',
        'native_timer_create_provider_selection_mismatch',
        'result.assignment.commandType==kBackendAgentNativeTimerCreateCommandType',
        'commandType==kBackendAgentNativeTimerCreateCommandType||',
        'backendAgentNativeTimerCreateParsePayload',
    ],
    "core/agent/src/BackendAgentCommandClient.cpp": [
        'BackendAgentNativeTimerCreate.h',
        'type == vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType ||',
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_delivery.cpp": [
        'Production Agent advertisement remains closed elsewhere',
        'local_provider_selection_stale',
        'persisted provider selection',
        'local_provider_selection_required',
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

client = (ROOT / "core/agent/src/BackendAgentCommandClient.cpp").read_text(
    encoding="utf-8"
)
availability_start = client.find("CommandAvailability availableCommands")
availability_end = client.find("\n}", availability_start)
availability = client[availability_start:availability_end]
closed = (
    availability_start >= 0
    and "kBackendAgentNativeTimerCreateCommandType" in availability
    and "kBackendAgentNativeTimerDeleteCommandType" in availability
    and "continue;" in availability
)
if not closed:
    errors.append("productive vdr.timer.create Agent advertisement must remain closed")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-delivery-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE delivery tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE delivery architecture guard passed")
