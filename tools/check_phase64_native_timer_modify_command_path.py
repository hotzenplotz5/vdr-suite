#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    target = ROOT / path
    if not target.is_file():
        raise SystemExit("missing productive modify path: " + path)
    return target.read_text(encoding="utf-8")

for path in (
    "core/agent/include/SuiteBridgeNativeTimerModifyTransport.h",
    "core/agent/src/SuiteBridgeSvdrpNativeTimerModifyTransport.cpp",
    "core/agent/include/BackendAgentNativeTimerModifyCommandHandler.h",
    "core/agent/src/BackendAgentNativeTimerModifyCommandHandler.cpp",
):
    read(path)

client = read("core/agent/src/BackendAgentCommandClient.cpp")
for token in (
    "nativeTimerModifyTransport",
    "timerModifyCommand",
    "kBackendAgentNativeTimerUpdateCommandType",
    "kBackendAgentNativeTimerToggleCommandType",
    "discoverProvider",
    "facts.available",
    "mergeProviderFacts",
):
    if token not in client:
        raise SystemExit("missing modify command client integration: " + token)

main = read("apps/agent/main.cpp")
if "SuiteBridgeNativeTimerModifyTransport" not in main:
    raise SystemExit("modify transport not constructed")

packaged = read("packaging/systemd/backend-agent.conf")
if "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete\n" not in packaged:
    raise SystemExit("modify command activation is not exact")

print("phase64 native Timer modify command path guard passed")
