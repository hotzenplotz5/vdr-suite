#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
paths=[
"core/agent/include/BackendAgentNativeTimerModify.h",
"core/agent/include/BackendAgentNativeTimerModifyPayload.h",
"core/agent/include/BackendAgentNativeTimerModifyLocalState.h",
"core/agent/include/BackendAgentNativeTimerModifyExecutor.h",
"core/agent/src/BackendAgentNativeTimerModify.cpp",
"core/agent/src/BackendAgentNativeTimerModifyPayload.cpp",
"core/agent/src/BackendAgentNativeTimerModifyLocalState.cpp",
"core/agent/src/BackendAgentNativeTimerModifyExecutor.cpp",
]
for path in paths:
 if not (ROOT/path).is_file(): raise SystemExit("missing modify Agent contract: "+path)
state=(ROOT/paths[6]).read_text(encoding="utf-8")
for token in ("localStartingPersistedAt","reconcileOnly","outcomeUnknown","payload_hex"):
 if token not in state: raise SystemExit("missing durable modify invariant: "+token)
executor=(ROOT/paths[7]).read_text(encoding="utf-8")
for token in ("discoverProvider","selectedProviderStillCurrent","modifyTimer"):
 if token not in executor: raise SystemExit("missing modify executor fence: "+token)
# A completed MODIFY must never inherit the DELETE-only evidence-free expiry
# exception, either during durable-state loading or during persistence.
store=(ROOT/"core/agent/src/BackendAgentCommandStateStore.cpp").read_text(encoding="utf-8")
if store.count("if (!deleteExpiredBeforeDispatch(state))") != 2:
 raise SystemExit("DELETE-only evidence-free exception leaked into another command")
delivery=(ROOT/"core/agent/src/BackendAgentCommandDelivery.cpp").read_text(encoding="utf-8")
if "native_timer_modify_result_evidence_invalid" not in delivery or \\
   "backendAgentNativeTimerModifyParseResultEvidence(" not in delivery:
 raise SystemExit("missing server-side MODIFY evidence admission guard")
print("phase64 native Timer modify Agent contract guard passed")
