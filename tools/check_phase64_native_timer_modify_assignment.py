#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/"core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp").read_text()
for token in ("ensureNativeTimerModifyAssignmentSchema","selectLocalProvider",
"insertAssignment","vdr.timer.update","vdr.timer.toggle","readback_required"):
 if token not in source:raise SystemExit("missing modify assignment invariant: "+token)
client=(ROOT/"core/agent/src/BackendAgentCommandClient.cpp").read_text()
if "kBackendAgentNativeTimerToggleCommandType" not in client:
 raise SystemExit("modify advertisement fence missing")
print("phase64 native Timer modify assignment guard passed")
