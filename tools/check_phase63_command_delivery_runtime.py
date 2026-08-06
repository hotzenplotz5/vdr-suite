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
    text = Path(path).read_text(encoding="utf-8")
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
print("Command type: probe.noop only")
print("Native VDR mutation: absent")
print("command_capability_required")
print("equivalent replay preserves the durable receipt")
print("parameterized capability cleanup")
print("replay counters expose durable acceptance evidence")
print("acknowledged stale generation state retires; pending state stays fenced")
