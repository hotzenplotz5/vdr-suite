#!/usr/bin/env python3
from pathlib import Path

required = {
    "core/agent/include/BackendAgentCommand.h": ["operationId", "jobId", "attemptId", "claimEpoch", "commandId", "requestFingerprint"],
    "core/agent/src/BackendAgentCommandDelivery.cpp": ["backend_agent_commands", "backend_agent_command_receipts", "backend_agent_command_results", "waiting_reconciliation", "probe.noop"],
    "core/agent/src/BackendAgentCommandClient.cpp": ["receiptAcknowledged", "resultAcknowledged", 'dispatchState="starting"', "outcome_unknown", "equivalent_assignment_replay"],
    "core/agent/src/BackendAgentHttpServer.cpp": ["/api/agent/v1/commands/poll", "/api/agent/v1/commands/receipt", "/api/agent/v1/commands/result"],
}
for path, markers in required.items():
    text = Path(path).read_text(encoding="utf-8")
    missing = [marker for marker in markers if marker not in text]
    if missing:
        raise SystemExit(f"{path}: missing {missing}")
for forbidden in ["timer.create", "recording.delete", "searchtimer.create", "providerUrl", "/api/v1/agent/"]:
    for path in ["core/agent/src/BackendAgentCommandDelivery.cpp", "core/agent/src/BackendAgentCommandClient.cpp"]:
        if forbidden in Path(path).read_text(encoding="utf-8"):
  raise SystemExit(f"{path}: forbidden runtime marker {forbidden}")
print("Phase-63 command delivery runtime check passed")
print("Command type: probe.noop only")
print("Native VDR mutation: absent")
