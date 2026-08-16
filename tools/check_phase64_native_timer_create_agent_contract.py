#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreate.h": [
        'kBackendAgentNativeTimerCreateCommandType = "vdr.timer.create"',
        'kBackendAgentNativeTimerCreateAuthorityDomain = "vdr.timer"',
        'kBackendAgentNativeTimerCreateProviderId = "suitebridge:local"',
        "expectedAssignmentRevision",
        "expectedIntentRevision",
        "assignmentEpoch",
        "nativeTimerBindingId",
        "expectedSpecificationFingerprint",
        "localProviderSelection",
    ],
    "core/agent/include/BackendAgentNativeTimerCreatePayload.h": [
        "BackendAgentNativeTimerCreatePayload",
        "backendAgentNativeTimerCreatePayloadValid",
        "backendAgentNativeTimerCreateParsePayload",
    ],
    "core/agent/src/BackendAgentNativeTimerCreatePayload.cpp": [
        'native-timer-create-agent-payload/1|',
        "kFieldCount = 28",
        "backendAgentNativeTimerCreatePayload(candidate) != encoded",
    ],
    "core/agent/src/BackendAgentCommand.cpp": [
        'BackendAgentNativeTimerCreatePayload.h',
        "kBackendAgentNativeTimerCreateCommandType",
        "backendAgentNativeTimerCreateParsePayload",
    ],
}

errors = []
for relative, markers in required.items():
    path = ROOT / relative
    if not path.exists():
        errors.append(f"missing {relative}")
        continue
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            errors.append(f"{relative}: missing marker {marker}")

agent_sources = (ROOT / "mk/agent-sources.mk").read_text(encoding="utf-8")
for source in (
    "core/agent/src/BackendAgentNativeTimerCreate.cpp",
    "core/agent/src/BackendAgentNativeTimerCreatePayload.cpp",
):
    if source not in agent_sources:
        errors.append(f"mk/agent-sources.mk: missing {source}")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-agent-contract-tests.mk" not in makefile:
    errors.append("Makefile: missing Phase 64 CREATE Agent contract tests include")

if errors:
    for error in errors:
        print(error)
    raise SystemExit(1)
print("Phase 64 native Timer CREATE Agent contract architecture guard passed")
