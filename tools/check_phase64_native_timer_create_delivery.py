#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

required = {
    "core/agent/include/BackendAgentNativeTimerCreate.h": [
        'kBackendAgentNativeTimerCreateCommandType = "vdr.timer.create"',
    ],
    "core/agent/src/BackendAgentCommandClient.cpp": [
        "kBackendAgentNativeTimerCreateCommandType",
        "nativeTimerCreateTransport",
        "discoverProvider",
        "mergeProviderFacts",
        "facts.available",
    ],
    "core/agent/tests/test_backend_agent_native_timer_create_delivery.cpp": [
        "An enabled, fenced provider advertisement makes CREATE deliverable",
        "local_provider_selection_stale",
        "persisted provider selection",
        "local_provider_selection_required",
    ],
}

errors = []
for relative, markers in required.items():
    path = ROOT / relative
    if not path.is_file():
        errors.append(f"missing {relative}")
        continue
    content = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in content:
            errors.append(f"{relative}: missing marker {marker}")

packaged = (ROOT / "packaging/systemd/backend-agent.conf").read_text(
    encoding="utf-8"
)
if "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete\n" not in packaged:
    errors.append("productive vdr.timer.create activation is not exact")

makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
if "include mk/phase64-native-timer-create-delivery-tests.mk" not in makefile:
    errors.append("Makefile: missing CREATE delivery tests include")

if errors:
    for error in errors:
        print(error, file=sys.stderr)
    raise SystemExit(1)

print("Phase 64 native Timer CREATE delivery guard passed")
