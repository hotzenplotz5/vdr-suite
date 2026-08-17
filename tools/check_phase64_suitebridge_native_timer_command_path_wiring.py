#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ACTIVATED = "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete"

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
delivery = read("core/agent/src/BackendAgentCommandDelivery.cpp")
advertisement = read(
    "core/agent/include/BackendAgentNativeTimerDeleteAdvertisement.h"
)
main = read("apps/agent/main.cpp")
runtime_mk = read("mk/backend-agent-runtime.mk")
packaged = read("packaging/systemd/backend-agent.conf")
client_test = read("core/agent/tests/test_backend_agent_command_client.cpp")
config_test = read("core/agent/tests/test_backend_agent_client.cpp")
svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
doc = read("docs/development/phase-64-suitebridge-native-timer-command-path-wiring.md")

include = "include mk/phase64-suitebridge-native-timer-command-path-wiring-tests.mk"
require(makefile, include, "command-path wiring Make include")
if makefile.count(include) != 1:
    raise SystemExit("command-path wiring Make include must occur exactly once")

for needle in (
    "suiteBridgeHost",
    "suiteBridgePort",
    "nativeTimerCreateTransport = nullptr",
    "nativeTimerDeleteTransport = nullptr",
    "nativeTimerModifyTransport = nullptr",
):
    require(header, needle, "default-closed transport injection")

for needle in (
    '"SUITEBRIDGE_HOST"',
    '"SUITEBRIDGE_PORT"',
    "loopbackHost(config.suiteBridgeHost)",
    "kBackendAgentNativeTimerCreateCommandType",
    "kBackendAgentNativeTimerUpdateCommandType",
    "kBackendAgentNativeTimerToggleCommandType",
    "kBackendAgentNativeTimerDeleteCommandType",
    "timerCommandsConfigured && config.suiteBridgeHost.empty()",
):
    require(client, needle, "Timer configuration fence")

for needle in (
    "SuiteBridgeNativeTimerCreateTransport",
    "SuiteBridgeNativeTimerDeleteTransport",
    "SuiteBridgeNativeTimerModifyTransport",
    "config.nativeTimerCreateTransport =",
    "config.nativeTimerDeleteTransport =",
    "config.nativeTimerModifyTransport =",
):
    require(main, needle, "installed Timer adapter wiring")

for needle in (
    "$(AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC)",
    "$(AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC)",
    "$(AGENT_NATIVE_TIMER_MODIFY_TRANSPORT_SRC)",
):
    require(runtime_mk, needle, "Timer adapter binary link")

if packaged.count("COMMAND_TYPES=") != 1:
    raise SystemExit("packaged command activation must occur exactly once")
require(packaged, ACTIVATED + "\n", "exact packaged Timer activation")

availability = command_client[
    command_client.find("bool mergeProviderFacts"):
    command_client.find("bool reconcileBackendAgentCommandState")
]
for needle in (
    "discoverProvider",
    "facts.available",
    "mergeProviderFacts",
    "timerSnapshotCoherent",
    "providerInstanceEpoch",
    "providerGeneration",
    "capabilityRevision",
):
    require(availability, needle, "runtime availability fence")
for token in (
    "kBackendAgentNativeTimerCreateCommandType",
    "kBackendAgentNativeTimerUpdateCommandType",
    "kBackendAgentNativeTimerToggleCommandType",
    "kBackendAgentNativeTimerDeleteCommandType",
):
    require(availability, token, "activated Timer command type")

for token in (
    "backendAgentNativeTimerAdvertisementValid",
    "native_timer_create_provider_advertisement_required",
    "native_timer_update_provider_advertisement_required",
    "native_timer_toggle_provider_advertisement_required",
    "native_timer_delete_provider_advertisement_required",
):
    require(advertisement + delivery, token, "Control Plane advertisement fence")

for token in (
    "kBackendAgentNativeTimerUpdateCommandType",
    "kBackendAgentNativeTimerToggleCommandType",
    "localProviderSelectionCurrent",
):
    require(delivery, token, "pre-delivery provider fence")

for token in (
    "testTimerAdvertisementActivation",
    "pie_timer_activation_replaced",
    "supportedCommandTypes.empty()",
    "localProviders.size() == 1",
):
    require(client_test, token, "activation regression")
require(config_test, ACTIVATED, "configuration activation regression")

help_start = svdrp.find("SVDRPHelpPages")
command_start = svdrp.find("SVDRPCommand", help_start)
if help_start < 0 or command_start < 0:
    raise SystemExit("SuiteBridge help boundary missing")
for command in ("NTCREATE", "NTMOD", "NTDELETE"):
    forbid(svdrp[help_start:command_start], command, "public Timer write help")

for token in (
    "passed the bounded real-yaVDR",
    "acceptance before activation",
    "Advertisement remains fail-closed at runtime",
    "public SuiteBridge SVDRP Help advertisement remains",
):
    require(doc, token, "activation documentation")

print("Phase 64 SuiteBridge native Timer command-path activation guard passed")
