#!/usr/bin/env python3

from pathlib import Path

required = {
    "core/agent/include/BackendAgentNativeTimerCreateExecutor.h": [
        "IBackendAgentNativeTimerCreateTransport",
        "backendAgentNativeTimerCreateExecuteFreshStartingOnce",
        "rejectedWithoutEffect",
        "acceptedUnverified",
        "outcomeUnknown",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateExecutor.cpp": [
        "executor-fence:agent-generation",
        "executor-fence:deadline",
        "executor-fence:provider-changed",
        "native_timer_create_executor_outcome_unknown",
        "transport.createTimer(request)",
    ],
    "core/agent/include/BackendAgentNativeTimerCreateCommandHandler.h": [
        "backendAgentNativeTimerCreateCommandReconcileExisting",
        "backendAgentNativeTimerCreateCommandPrepareFreshStarting",
        "backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome",
    ],
    "core/agent/src/BackendAgentNativeTimerCreateCommandHandler.cpp": [
        "native_create_local_starting_persisted",
        "native_create_executor_outcome_persisted",
        "BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_executor.cpp": [
        "forbiddenReplay.createCalls == 0",
        "acceptedUnverified",
        "outcomeUnknown",
        "changedProvider.createCalls == 0",
    ],
}

for filename, markers in required.items():
    path = Path(filename)
    if not path.is_file():
        raise SystemExit(f"missing required file: {filename}")

    text = path.read_text()

    for marker in markers:
        if marker not in text:
            raise SystemExit(
                f"{filename}: missing architecture marker: {marker}"
            )

client = Path("core/agent/src/BackendAgentCommandClient.cpp").read_text()

if "kBackendAgentNativeTimerCreateCommandType" not in client:
    raise SystemExit("CREATE command type missing from command client")

if "config.nativeTimerCreateTransport" not in client:
    raise SystemExit("CREATE transport not wired into command client")

# Shipping boundary stays closed in this slice. CREATE must still be filtered
# from the advertised polling capability until exact-candidate yaVDR acceptance.
availability_start = client.find("CommandAvailability availableCommands")
availability_end = client.find(
    "bool reconcileBackendAgentCommandState",
    availability_start,
)

if availability_start < 0 or availability_end < 0:
    raise SystemExit("command availability function not found")

availability = client[availability_start:availability_end]

if (
    "kBackendAgentNativeTimerCreateCommandType" not in availability
    or "continue;" not in availability
):
    raise SystemExit(
        "CREATE capability advertisement is not visibly held closed"
    )

if Path(
    "core/agent/src/SuiteBridgeSvdrpNativeTimerCreateTransport.cpp"
).exists():
    raise SystemExit(
        "SuiteBridge CREATE transport must not land in executor slice"
    )

sources = Path("mk/agent-sources.mk").read_text()

for marker in (
    "AGENT_TIMER_CREATE_EXECUTOR_SRC",
    "BackendAgentNativeTimerCreateExecutor.cpp",
    "AGENT_NATIVE_TIMER_CREATE_COMMAND_HANDLER_SRC",
    "BackendAgentNativeTimerCreateCommandHandler.cpp",
    "BackendAgentNativeTimerCreateRecovery.cpp",
):
    if marker not in sources:
        raise SystemExit(
            f"mk/agent-sources.mk missing runtime marker: {marker}"
        )

print(
    "Phase 64 native Timer CREATE executor architecture guard passed"
)
