#!/usr/bin/env python3
from pathlib import Path

required = {
    "core/agent/include/BackendAgentCommand.h": [
        "operationId", "jobId", "attemptId", "claimEpoch",
        "commandId", "requestFingerprint",
    ],
    "core/agent/include/BackendAgentCommandDelivery.h": [
        "hasCapability",
    ],
    "core/agent/src/BackendAgentCommandDelivery.cpp": [
        "backend_agent_commands",
        "backend_agent_command_receipts",
        "backend_agent_command_results",
        "waiting_reconciliation",
        "probe.noop",
        "command_capability_required",
        "DELETE FROM backend_agent_command_capabilities WHERE backend_id=?",
                  "delivery_count=delivery_count+1",
                  "receipt_replay_count=receipt_replay_count+1",
                  "result_replay_count=result_replay_count+1",
    ],
    "core/agent/src/BackendAgentCommandClient.cpp": [
        "receiptAcknowledged",
        "resultAcknowledged",
        'dispatchState="starting"',
        "outcome_unknown",
        "current.receiptAcknowledged=false",
    ],
    "core/agent/tests/test_backend_agent_command_client.cpp": [
                  "completed_command_state_retired",
                  "local_command_generation_fenced",
              ],
              "core/agent/src/BackendAgentHttpServer.cpp": [
        "/api/agent/v1/commands/poll",
        "/api/agent/v1/commands/receipt",
        "/api/agent/v1/commands/result",
    ],
    "core/agent/tests/test_backend_agent_command_delivery.cpp": [
        "command_capability_required",
        "capabilityPoll.accepted",
                  "receiptReplayCount==1",
                  "resultReplayCount==1",
    ],
}

for path, markers in required.items():
    text = Path(path).read_text(encoding="utf-8")
    missing = [marker for marker in markers if marker not in text]
    if missing:
        raise SystemExit(f"{path}: missing {missing}")

delivery_path = "core/agent/src/BackendAgentCommandDelivery.cpp"
delivery_boundary = Path(delivery_path).read_text(encoding="utf-8")
timer_delivery_successor = (
    "c.command_type NOT IN('vdr.native.probe','vdr.timer.create',"
    "'vdr.timer.update','vdr.timer.toggle','vdr.timer.delete')"
)
if "timer.create" in delivery_boundary:
    activation_guard_path = Path(
        "tools/check_phase64_suitebridge_native_timer_command_path_wiring.py"
    )
    activation_guard = (
        activation_guard_path.read_text(encoding="utf-8")
        if activation_guard_path.is_file()
        else ""
    )
    packaged = Path("packaging/systemd/backend-agent.conf").read_text(
        encoding="utf-8"
    )
    if (
        delivery_boundary.count(timer_delivery_successor) != 1
        or packaged.count(
            "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,"
            "vdr.timer.toggle,vdr.timer.delete\n"
        ) != 1
    ):
        raise SystemExit(
            f"{delivery_path}: Timer successor activation is not exact"
        )
    for marker in [
        '"native_timer_delete_provider_advertisement_required"',
        '"localProviderSelectionCurrent"',
        '"kBackendAgentNativeTimerDeleteCommandType"',
    ]:
        if marker not in activation_guard:
            raise SystemExit(
                f"{delivery_path}: Timer successor guard misses {marker}"
            )
    for marker in [
        "JOIN backend_agent_local_provider_ownership",
        "JOIN backend_agent_local_provider_facts",
        "f.provider_instance_epoch=s.provider_instance_epoch",
        "f.provider_generation=s.provider_generation",
        "f.capability_revision=s.capability_revision",
        "f.available=1",
    ]:
        if marker not in delivery_boundary:
            raise SystemExit(
                f"{delivery_path}: Timer successor fence misses {marker}"
            )
    delivery_boundary = delivery_boundary.replace(
        timer_delivery_successor, "", 1
    )

forbidden_by_path = {
    "core/agent/src/BackendAgentCommandDelivery.cpp": [
        "DELETE FROM backend_agent_command_capabilities WHERE backend_id='",
        "timer.create", "recording.delete", "searchtimer.create", "providerUrl",
    ],
    "core/agent/src/BackendAgentCommandClient.cpp": [
        'receiptCategory="duplicate"',
        'reasonCode="equivalent_assignment_replay"',
        "timer.create", "recording.delete", "searchtimer.create", "providerUrl",
    ],
}
for path, markers in forbidden_by_path.items():
    text = (
        delivery_boundary
        if path == delivery_path
        else Path(path).read_text(encoding="utf-8")
    )
    for marker in markers:
        if marker in text:
            raise SystemExit(f"{path}: forbidden runtime marker {marker}")

for path in [
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    "core/agent/src/BackendAgentCommandClient.cpp",
]:
    if "/api/v1/agent/" in Path(path).read_text(encoding="utf-8"):
        raise SystemExit(f"{path}: public Agent route marker")

print("Phase-63 command delivery runtime check passed")
print("Command type: probe.noop baseline plus gated Phase-64 Timer successor")
print("Native VDR mutation: absent")
print("command_capability_required")
print("equivalent replay preserves the durable receipt")
print("parameterized capability cleanup")
print("replay counters expose durable acceptance evidence")
print("acknowledged stale generation state retires; pending state stays fenced")
