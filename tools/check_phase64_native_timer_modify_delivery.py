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
print("phase64 native Timer modify delivery guard passed")
