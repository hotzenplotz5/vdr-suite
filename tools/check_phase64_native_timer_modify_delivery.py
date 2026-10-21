#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
command=read("core/agent/src/BackendAgentCommand.cpp")
json=read("core/agent/src/BackendAgentCommandJson.cpp")
delivery=read("core/agent/src/BackendAgentCommandDelivery.cpp")
for token in ("BackendAgentNativeTimerModifyPayload",
"kBackendAgentNativeTimerUpdateCommandType","kBackendAgentNativeTimerToggleCommandType"):
 if token not in command:raise SystemExit("modify assignment validation missing: "+token)
 if token not in delivery:raise SystemExit("modify delivery missing: "+token)
for token in ("kBackendAgentNativeTimerUpdateCommandType",
"kBackendAgentNativeTimerToggleCommandType"):
 if token not in json:raise SystemExit("modify poll parser missing: "+token)
client=read("core/agent/src/BackendAgentCommandClient.cpp")
start=client.find("CommandAvailability availableCommands")
end=client.find("\n}",start)
for token in ("kBackendAgentNativeTimerUpdateCommandType",
"kBackendAgentNativeTimerToggleCommandType","continue;"):
 if token not in client[start:end]:raise SystemExit("modify advertisement fence missing: "+token)
# Prevent durable MODIFY results without executor-bound evidence.
# CREATE already enforces this contract; UPDATE/TOGGLE must not regress.
modify_branch=delivery.split(
    "else if(commandType==kBackendAgentNativeTimerUpdateCommandType||",1)
if len(modify_branch)!=2:
 raise SystemExit("MODIFY result intake branch missing")
modify_branch=modify_branch[1].split(
    "else if(commandType==kBackendAgentRecordingMarksModifyCommandType)",1)[0]
for token in (
    "backendAgentNativeTimerModifyParseResultEvidence(",
    "r.resultEvidence.empty()",
    "evidence.completedAt != r.completedAt",
    "native_timer_modify_result_evidence_invalid",
    "native_timer_modify_result_evidence_conflict",
    "BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect",
    "BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified",
    "BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown",
    "projectionMatches",
):
 if token not in modify_branch:
  raise SystemExit("MODIFY result evidence intake guard missing: "+token)
# Do not fabricate evidence for previously committed legacy rows.
if "UPDATE backend_agent_command_results SET result_evidence" in delivery:
 raise SystemExit("forbidden retroactive MODIFY evidence fabrication")
print("phase64 native Timer modify delivery guard passed")
