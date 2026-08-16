#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreateLocalState.h": [
        "BackendAgentNativeTimerCreateLocalPhase",
        "BackendAgentNativeTimerCreateRecoveryDecision",
        "reconcileOnly",
        "backendAgentNativeTimerCreatePrepareLocalStarting",
        "backendAgentNativeTimerCreateRecoverLocalState",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp": [
        'native-timer-create-local-state/1|',
        "native_timer_create_dispatch_may_have_occurred_reconcile_only",
        "backendAgentNativeTimerCreateEvidenceMatches",
        "backendAgentNativeTimerCreateParsePayload",
        "backendAgentNativeTimerCreateSerializeLocalState(candidate, reasonCode) != encoded",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_local_state.cpp": [
        "must never authorize a second dispatch",
        "BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly",
        "acceptedUnverified",
        "outcomeUnknown",
        "wrongBinding",
        "wrongProvider",
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

source = (ROOT / "core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp").read_text(
    encoding="utf-8"
)
for forbidden in (
    "createTimer(",
    "deleteTimer(",
    "backendAgentGenerateOpaqueId",
):
    if forbidden in source:
        errors.append(f"CREATE local-state layer must not dispatch native mutation: {forbidden}")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-local-state-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE local-state tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE local-state architecture guard passed")
