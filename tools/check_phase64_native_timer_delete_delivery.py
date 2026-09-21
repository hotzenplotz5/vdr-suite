#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 26 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
delivery = read("core/agent/src/BackendAgentCommandDelivery.cpp")
command_json = read("core/agent/src/BackendAgentCommandJson.cpp")
advertisement = read("core/agent/include/BackendAgentNativeTimerDeleteAdvertisement.h")
test = read("core/agent/tests/test_backend_agent_native_timer_delete_delivery.cpp")
doc = read("docs/development/phase-64-native-timer-delete-delivery.md")
mk = read("mk/phase64-native-timer-delete-delivery-tests.mk")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")
agent_sources = read("mk/agent-sources.mk")

require(
    makefile,
    "include mk/phase64-native-timer-delete-delivery-tests.mk",
    "Slice 26 Make include",
)
if makefile.count("include mk/phase64-native-timer-delete-delivery-tests.mk") != 1:
    raise SystemExit("Slice 26 Make include must occur exactly once")

require(
    advertisement,
    "backendAgentNativeTimerDeleteAdvertisementValid",
    "bounded Timer-delete advertisement validator",
)
require(
    advertisement,
    "kBackendAgentNativeTimerDeleteProviderId",
    "exact SuiteBridge provider ID fence",
)
require(
    advertisement,
    "kBackendAgentNativeTimerDeleteCapability",
    "Timer-delete provider capability fence",
)
require(advertisement, "provider->available", "provider availability fence")

require(
    command_json,
    "kBackendAgentNativeTimerDeleteCommandType",
    "Timer-delete poll JSON command type",
)
require(
    delivery,
    '#include "BackendAgentNativeTimerDeleteAdvertisement.h"',
    "delivery advertisement contract include",
)
require(
    delivery,
    '#include "BackendAgentNativeTimerDeletePayload.h"',
    "Timer-delete result payload include",
)
require(
    delivery,
    "backendAgentNativeTimerAdvertisementValid(request,advertisementReason)",
    "server-side advertisement validation",
)
require(
    delivery,
    "DROP TRIGGER IF EXISTS trg_backend_agent_timer_delete_dormant_capability",
    "Slice 25 dormant gate retirement",
)
require(
    delivery,
    "native_timer_delete_provider_selection_mismatch",
    "exact Timer-delete provider-selection domain fence",
)
require(
    delivery,
    "result.assignment.commandType==kBackendAgentNativeTimerDeleteCommandType",
    "Timer-delete delivery current-fence check",
)
require(
    delivery,
    "else if(commandType==kBackendAgentNativeTimerDeleteCommandType)",
    "Timer-delete receipt/result handling",
)
require(
    delivery,
    "backendAgentNativeTimerDeleteParsePayload(payload,deletePayload,payloadReason)",
    "Timer-delete result sidecar correlation",
)

require(test, "local_provider_selection_stale", "provider drift coverage")
require(test, "replayedReceipt.replayed", "lost-response receipt replay coverage")
require(test, "Slice 26 has no local vdr.timer.delete executor", "non-dispatch result coverage")
require(test, "probe.noop", "adjacent non-mutating delivery regression")
require(mk, "test-phase64-native-timer-delete-delivery", "focused Slice 26 target")
require(doc, "Availability is not authority", "Slice 26 authority boundary documentation")
require(doc, "no local Timer-delete executor", "executor scope boundary documentation")

# The historical protocol boundary is preserved by the accepted successor:
# production delivery requires enabled provider discovery and an exact package opt-in.
require(command_client, "discoverProvider", "Timer-delete provider discovery")
require(command_client, "facts.available", "Timer-delete availability fence")
require(agent_client, "kBackendAgentNativeTimerDeleteCommandType",
        "Timer-delete Agent configuration allowlist")
require(packaged_config, "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete", "accepted packaged Timer activation")
forbid(
    agent_sources,
    "BackendAgentNativeTimerDeleteAssignment.cpp",
    "Timer-delete assignment runtime source wiring",
)

for token in (
    "SuiteBridgeSvdrp",
    "restfulapi",
    "system(",
    "popen(",
    "SVDRP",
):
    forbid(advertisement, token, "generic/native transport coupling in advertisement contract")

print("Phase 64 native Timer delete delivery architecture guard passed")
