#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(p):return (ROOT/p).read_text(encoding="utf-8")
for p in ("core/agent/include/SuiteBridgeNativeTimerModifyTransport.h",
"core/agent/src/SuiteBridgeSvdrpNativeTimerModifyTransport.cpp",
"core/agent/include/BackendAgentNativeTimerModifyCommandHandler.h",
"core/agent/src/BackendAgentNativeTimerModifyCommandHandler.cpp"):
 if not (ROOT/p).is_file():raise SystemExit("missing productive modify path: "+p)
client=read("core/agent/src/BackendAgentCommandClient.cpp")
for token in ("nativeTimerModifyTransport","timerModifyCommand",
"kBackendAgentNativeTimerUpdateCommandType","kBackendAgentNativeTimerToggleCommandType"):
 if token not in client:raise SystemExit("missing modify command client integration: "+token)
main=read("apps/agent/main.cpp")
if "SuiteBridgeNativeTimerModifyTransport" not in main:raise SystemExit("modify transport not constructed")
if 'type == vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType' not in client:
 raise SystemExit("UPDATE advertisement not closed")
print("phase64 native Timer modify command path guard passed")
