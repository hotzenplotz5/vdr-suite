#!/usr/bin/env python3
"""Phase 68.G allowlisted Legacy OSD native-input architecture guard."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
errors = []


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        errors.append(f"missing Phase-68.G file: {path}")
        return ""
    return target.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        errors.append(message)


domain = read("core/vdr/include/LegacyOsdInputDomain.h")
service = read("core/daemon/src/LegacyOsdInputService.cpp")
service_h = read("core/daemon/include/LegacyOsdInputService.h")
api = read("api/rest/src/LegacyOsdApiRuntime.cpp")
security = read("core/security/include/SecurityHttpGate.h")
delivery = read("core/agent/src/BackendAgentCommandDelivery.cpp")
client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_runtime = read("core/agent/src/BackendAgentClient.cpp")
agent_runtime_h = read("core/agent/include/BackendAgentClient.h")
agent_main = read("apps/agent/main.cpp")
agent_test = read("core/agent/tests/test_backend_agent_client.cpp")
transport = read("core/agent/include/SuiteBridgeSvdrpTransport.h")
plugin = read("vdr-plugin-suite-bridge/suitebridge_osd_input.cpp")
plugin_svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
caps = read("vdr-plugin-suite-bridge/suitebridge_capabilities.cpp")
daemon = read("core/daemon/src/DaemonLegacyOsdRuntime.cpp")
make = read("mk/phase68-legacy-osd-tests.mk")

for action in (
    "up", "down", "left", "right", "ok", "back",
    "red", "green", "yellow", "blue",
):
    require(
        f'== "{action}"' in domain or f'case LegacyOsdInputAction::' in domain,
        f"normalized allowlist missing action: {action}",
    )

for forbidden_action in (
    '"menu"', '"play"', '"pause"', '"stop"',
    '"volume_up"', '"volume_down"', '"number_0"', '"number_9"',
):
    require(
        forbidden_action not in plugin,
        f"first 68.G plugin allowlist grew beyond SB.15: {forbidden_action}",
    )

for token in (
    'kLegacyOsdInputCommandType = "vdr.legacy-osd.input"',
    "controllerLeaseEpoch",
    "leaseRevision",
    "backendGeneration",
    "osdSurfaceId",
    "osdEpoch",
    'inputMode = "press"',
    "repeatCount = 1",
    "deadline",
):
    require(token in domain, f"input domain missing fence: {token}")

for token in (
    "controllerService_.current(",
    "current.lease.controllerLeaseId != command.controllerLeaseId",
    "current.lease.controllerLeaseEpoch != command.controllerLeaseEpoch",
    "current.lease.leaseRevision != command.leaseRevision",
    "current.lease.backendGeneration != command.backendGeneration",
    "current.lease.osdSurfaceId != command.osdSurfaceId",
    "current.lease.osdEpoch != command.osdEpoch",
    "MaximumDeadlineLeadSeconds = 3",
    "MaximumAcceptedCommandsPerSecond = 12",
    "MaximumRateWindows = 128",
    "rateWindows_.size() >= MaximumRateWindows",
    "rateWindows_.erase(",
    'verificationPolicy = "dispatch_only"',
    "findAssignmentForOperation(",
):
    require(token in service_h + service, f"input service missing fence: {token}")

for token in (
    '"/api/vdr/legacy-osd/input"',
    "parseInputBody(",
    "inputService_->submit(command)",
    "jsonResponse(202",
):
    require(token in api, f"input API missing contract: {token}")

require(
    "isLegacyOsdInput" in security and
    'requestToAuthorize.permission = "osd.control"' in security and
    '"osd.input"' in security,
    "input route must remain protected by osd.control",
)
protected = security[
    security.find("const bool isProtectedMutation"):
    security.find("const bool isExplicitlyAuthorizedPost")
]
require(
    "isLegacyOsdInput" in protected,
    "input route must be a protected mutation so browser CSRF applies",
)

for token in (
    "setReceiptFenceCheck(",
    "legacy-osd.input.rejected",
    "legacy-osd.input.accepted",
    "dispatch_fence_current",
):
    require(token in delivery + daemon, f"receipt dispatch fence missing: {token}")

receipt_persist = delivery.find(
    "result = commandRepository_.acceptReceipt(receipt);")
receipt_accept_audit = delivery.find(
    '"legacy-osd.input.accepted"', receipt_persist)
require(
    receipt_persist >= 0 and receipt_accept_audit > receipt_persist,
    "accepted input audit must follow durable receipt acceptance",
)
result_persist = delivery.find(
    "result = commandRepository_.acceptResult(value);")
result_semantic_audit = delivery.find(
    '"native_dispatch_reported"', result_persist)
require(
    result_persist >= 0 and result_semantic_audit > result_persist,
    "native input result audit must follow durable result acceptance",
)

for token in (
    "state.dispatchState = \"starting\"",
    "if (state.dispatchState != \"not_started\")",
    "outcome_unknown",
    "reconcile_only",
    "executeLegacyOsdInput(",
):
    require(token in client, f"no-blind-retry Agent behavior missing: {token}")

for token in (
    "commandPollIntervalMilliseconds = 0",
    "SleepMilliseconds",
    "pollCommands(std::string& reasonCode)",
    "sleepMilliseconds_(waitMilliseconds)",
):
    require(
        token in agent_runtime_h + agent_runtime,
        f"low-latency Agent polling contract missing: {token}",
    )
require(
    "config.commandPollIntervalMilliseconds = 250" in agent_main,
    "Legacy OSD input must activate bounded 250ms command polling",
)
require(
    "test_low_latency_command_poll_between_heartbeats" in agent_test,
    "low-latency command polling must be covered by the Agent client test",
)

require(
    '"PLUG suitebridge OSDINPUT 1 "' in transport,
    "Agent must use one typed private OSDINPUT request",
)
require(
    'strcasecmp(command, "OSDINPUT")' in plugin,
    "SuiteBridge must accept only the named OSDINPUT operation",
)
for native_key in (
    "kUp", "kDown", "kLeft", "kRight", "kOk",
    "kBack", "kRed", "kGreen", "kYellow", "kBlue",
):
    require(native_key in plugin, f"native key mapping missing: {native_key}")
require(
    "cRemote::Put(key)" in plugin,
    "typed input service must terminate at VDR's native remote input primitive",
)
require(
    "snapshot.active" in plugin and
    "snapshot.complete" in plugin and
    "snapshot.consistent" in plugin and
    "osdEpoch != snapshot.osdEpoch.data()" in plugin,
    "plugin must revalidate current local OSD epoch before dispatch",
)
require(
    "RecentCommandCapacity = 64" in
    read("vdr-plugin-suite-bridge/suitebridge_osd_input.h"),
    "plugin duplicate memory must remain bounded",
)
require(
    '{"osd.control", SuiteBridgeCapabilityState::Available}' in caps,
    "SuiteBridge osd.control capability must be truthful for 68.G",
)
require(
    '"OSDINPUT <schema> <command-id>' in plugin_svdrp,
    "private SVDRP help must document only the typed OSDINPUT schema",
)

public_side = "\n".join((domain, service, service_h, api, security, delivery, client))
for forbidden in (
    "pressRawKey",
    "runSvdrp",
    "callPlugin",
    "sendKeyboard",
    "execute(command)",
    "rawKeyCode",
    "keyCode",
    "system(",
    "popen(",
    "fork(",
    "execv(",
):
    require(
        forbidden not in public_side,
        f"generic/raw command tunnel forbidden in public/control path: {forbidden}",
    )

require(
    "test-phase68-osd-native-input" in make,
    "Phase 68.G focused tests must be in the Phase-68 Make graph",
)

if errors:
    for error in errors:
        print(f"ERROR={error}")
    raise SystemExit(1)

print("RESULT=PHASE68G_OSD_NATIVE_INPUT_CONTRACT_PASS")
