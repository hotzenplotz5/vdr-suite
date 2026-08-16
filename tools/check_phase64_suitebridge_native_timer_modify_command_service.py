#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def read(path): return (ROOT/path).read_text(encoding="utf-8")
for path in (
 "vdr-plugin-suite-bridge/suitebridge_native_timer_modify.h",
 "vdr-plugin-suite-bridge/suitebridge_native_timer_modify.cpp",
 "vdr-plugin-suite-bridge/suitebridge_native_timer_modify_vdr.h",
 "vdr-plugin-suite-bridge/suitebridge_native_timer_modify_vdr.cpp",
 "vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_modify.cpp",
):
    if not (ROOT/path).is_file(): raise SystemExit("missing modify service: "+path)
source=read("vdr-plugin-suite-bridge/suitebridge_native_timer_modify.cpp")
for token in ("NTMOD","vdr.timer.update","vdr.timer.toggle","localStartingPersistedAt",
              "replayByOperationId_","callbackResult"):
    if token not in source: raise SystemExit("missing modify fence: "+token)
vdr=read("vdr-plugin-suite-bridge/suitebridge_native_timer_modify_vdr.cpp")
for token in ("GetTimersWrite","expectedNativeTimerFingerprint","Recording()",
              "Pending()","SetExplicitModify","SetModified"):
    if token not in vdr: raise SystemExit("missing VDR mutation guard: "+token)
plugin=read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
help_start=plugin.find("SVDRPHelpPages")
command_start=plugin.find("SVDRPCommand",help_start)
if help_start<0 or command_start<0: raise SystemExit("help boundary missing")
if "NTMOD" in plugin[help_start:command_start]:
    raise SystemExit("public NTMOD help advertisement landed before acceptance")
if "nativeTimerModify_.Handle(Command, Option)" not in plugin:
    raise SystemExit("private modify dispatch missing")
agent=read("core/agent/src/BackendAgentCommandClient.cpp")
for token in ("kBackendAgentNativeTimerCreateCommandType",
              "kBackendAgentNativeTimerDeleteCommandType"):
    if token not in agent: raise SystemExit("closed command advertisement guard missing")
print("phase64 SuiteBridge native timer modify command service guard passed")
