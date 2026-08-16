#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing native Timer command-path wiring file: {path}")
    return target.read_text(encoding="utf-8")

def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")

def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")

makefile = read("Makefile")
header = read("core/agent/include/BackendAgentClient.h")
client = read("core/agent/src/BackendAgentClient.cpp")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
main = read("apps/agent/main.cpp")
runtime_mk = read("mk/backend-agent-runtime.mk")
packaged = read("packaging/systemd/backend-agent.conf")
test = read("core/agent/tests/test_backend_agent_client.cpp")
doc = read("docs/development/phase-64-suitebridge-native-timer-command-path-wiring.md")

include = "include mk/phase64-suitebridge-native-timer-command-path-wiring-tests.mk"
require(makefile, include, "command-path wiring Make include")
if makefile.count(include) != 1:
    raise SystemExit("command-path wiring Make include must occur exactly once")

for needle, label in (
    ("suiteBridgeHost", "loopback SuiteBridge host configuration"),
    ("suiteBridgePort", "SuiteBridge port configuration"),
    ("nativeTimerCreateTransport = nullptr", "default-closed CREATE injection"),
    ("nativeTimerDeleteTransport = nullptr", "default-closed DELETE injection"),
):
    require(header, needle, label)

for needle, label in (
    ('"SUITEBRIDGE_HOST"', "SuiteBridge host parser"),
    ('"SUITEBRIDGE_PORT"', "SuiteBridge port parser"),
    ("loopbackHost(config.suiteBridgeHost)", "loopback-only SuiteBridge fence"),
    ("config_.nativeTimerCreateTransport", "CREATE CommandClient injection"),
    ("config_.nativeTimerDeleteTransport", "DELETE CommandClient injection"),
):
    require(client, needle, label)

for needle, label in (
    ('#include "SuiteBridgeNativeTimerCreateTransport.h"', "CREATE adapter include"),
    ('#include "SuiteBridgeNativeTimerDeleteTransport.h"', "DELETE adapter include"),
    ("SuiteBridgeNativeTimerCreateTransport", "CREATE adapter construction"),
    ("SuiteBridgeNativeTimerDeleteTransport", "DELETE adapter construction"),
    ("config.nativeTimerCreateTransport =", "CREATE runtime injection"),
    ("config.nativeTimerDeleteTransport =", "DELETE runtime injection"),
):
    require(main, needle, label)

for needle, label in (
    ("$(AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC)", "CREATE adapter binary link"),
    ("$(AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC)", "DELETE adapter binary link"),
):
    require(runtime_mk, needle, label)

for needle, label in (
    ("SUITEBRIDGE_HOST=127.0.0.1", "packaged loopback host"),
    ("SUITEBRIDGE_PORT=6419", "packaged SuiteBridge port"),
    ("COMMAND_TYPES=", "closed packaged command advertisement"),
):
    require(packaged, needle, label)
for token in ("vdr.timer.create", "vdr.timer.delete"):
    forbid(packaged, token, "premature packaged Timer command advertisement")

availability = command_client[
    command_client.find("CommandAvailability availableCommands"):
    command_client.find("bool reconcileBackendAgentCommandState")
]
for needle in (
    "kBackendAgentNativeTimerCreateCommandType",
    "kBackendAgentNativeTimerDeleteCommandType",
    "continue;",
):
    require(availability, needle, "closed Timer advertisement fence")

for needle in (
    "SUITEBRIDGE_HOST=127.0.0.1",
    "SUITEBRIDGE_PORT=6419",
    'config.suiteBridgeHost == "127.0.0.1"',
    "config.suiteBridgePort == 6419",
    "invalid_suitebridge_configuration",
):
    require(test, needle, "SuiteBridge configuration regression")

require(doc, "advertisement remains closed", "closed advertisement documentation")
require(doc, "Control Plane", "productive command path documentation")
print("Phase 64 SuiteBridge native Timer command-path wiring guard passed")
