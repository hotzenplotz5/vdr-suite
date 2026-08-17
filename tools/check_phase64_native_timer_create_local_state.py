#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreateLocalState.h": [
        "BackendAgentNativeTimerCreateLocalPhase",
        "BackendAgentNativeTimerCreateRecoveryDecision",
        "BackendAgentNativeTimerCreateRecoveryResult",
        "reconcileOnly",
        "backendAgentNativeTimerCreatePrepareLocalStarting",
        "backendAgentNativeTimerCreateRecoverLocalState",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp": [
        'native-timer-create-local-state/1|',
        "backendAgentNativeTimerCreateEvidenceMatches",
        "backendAgentNativeTimerCreateParsePayload",
        "backendAgentNativeTimerCreateSerializeLocalState(candidate, reasonCode) != encoded",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp": [
        "no-blind-retry boundary",
        "BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown",
        "evidence.dispatchStartedAt = state.localStartingPersistedAt",
        'evidence.evidenceReference = "local-recovery:" + command.commandId',
        "native_timer_create_starting_recovery_reconcile_only",
        "native_timer_create_starting_context_fenced_reconcile_only",
        "native_timer_create_completed_evidence_survives_context_drift",
        "backendAgentNativeTimerCreateEvidenceMatches",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_local_state.cpp": [
        "must never authorize a second dispatch",
        "BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly",
        "local-recovery:cmd_create_local_1",
        "native_timer_create_starting_context_fenced_reconcile_only",
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

for relative in (
    "core/agent/src/BackendAgentNativeTimerCreateLocalState.cpp",
    "core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp",
):
    source = (ROOT / relative).read_text(encoding="utf-8")
    for forbidden in (
        "createTimer(",
        "deleteTimer(",
        "backendAgentGenerateOpaqueId",
    ):
        if forbidden in source:
            errors.append(
                f"CREATE local-state/recovery layer must not dispatch native mutation: {relative}: {forbidden}"
            )

make_fragment = (ROOT / "mk/phase64-native-timer-create-local-state-tests.mk").read_text(
    encoding="utf-8"
)
if "core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp" not in make_fragment:
    errors.append("CREATE local-state test must link conservative recovery implementation")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-local-state-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE local-state tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE local-state architecture guard passed")
