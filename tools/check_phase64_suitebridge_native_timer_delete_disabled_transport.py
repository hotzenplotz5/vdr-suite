#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 34 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
agent_sources = read("mk/agent-sources.mk")
svdrp_header = read("core/agent/include/SuiteBridgeSvdrpTransport.h")
adapter_header = read("core/agent/include/SuiteBridgeNativeTimerDeleteTransport.h")
transport_source = read(
    "core/agent/src/SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp"
)
agent_test = read(
    "core/agent/tests/test_suite_bridge_svdrp_native_timer_delete_transport.cpp"
)
plugin_header = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete.h")
plugin_source = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete.cpp")
plugin_test = read(
    "vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_delete.cpp"
)
plugin_main_header = read("vdr-plugin-suite-bridge/suitebridge.h")
plugin_main = read("vdr-plugin-suite-bridge/suitebridge.cpp")
plugin_svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
plugin_makefile = read("vdr-plugin-suite-bridge/Makefile")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")
mk = read("mk/phase64-suitebridge-native-timer-delete-disabled-transport-tests.mk")
doc = read(
    "docs/development/phase-64-suitebridge-native-timer-delete-disabled-transport.md"
)

include = "include mk/phase64-suitebridge-native-timer-delete-disabled-transport-tests.mk"
require(makefile, include, "Slice 34 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 34 Make include must occur exactly once")

source_path = "core/agent/src/SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp"
require(
    agent_sources,
    "AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC :=",
    "dedicated typed Timer-delete transport source set",
)
if agent_sources.count(source_path) != 1:
    raise SystemExit("typed SuiteBridge Timer-delete transport source must occur exactly once")
transport_block = agent_sources.split(
    "AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC :=", 1
)[1].split("\n\nAGENT_OBSERVATION_SRC", 1)[0]
require(transport_block, source_path, "dedicated Timer-delete transport source ownership")
generic_block = agent_sources.split("AGENT_SVDRP_TRANSPORT_SRC :=", 1)[1].split(
    "\n\nAGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC", 1
)[0]
forbid(generic_block, source_path, "Timer-delete source in generic SVDRP source set")

forbid(
    svdrp_header,
    "public IBackendAgentNativeTimerDeleteTransport",
    "Timer-delete interface on generic SuiteBridge SVDRP transport",
)
require(
    svdrp_header,
    "SuiteBridgeCommandReply discoverNativeTimerDeleteContract();",
    "typed raw capability hook",
)
require(
    svdrp_header,
    "SuiteBridgeCommandReply executeNativeTimerDeleteContract(",
    "typed raw execute hook",
)
require(
    adapter_header,
    "class SuiteBridgeNativeTimerDeleteTransport final :",
    "dedicated Timer-delete adapter",
)
require(
    adapter_header,
    "public IBackendAgentNativeTimerDeleteTransport",
    "typed Timer-delete interface implementation",
)
require(adapter_header, "SuiteBridgeSvdrpTransport transport_;", "composed SVDRP transport")

for needle, label in (
    ('"PLUG suitebridge NTDEL CAP 1\\r\\n"', "typed capability wire"),
    ('"PLUG suitebridge NTDEL EXEC vdr-suite-native/1 "', "typed execute wire"),
    ("command.commandId", "command id wire identity"),
    ("command.requestFingerprint", "request fingerprint wire identity"),
    ("command.operationId", "operation id wire identity"),
    ("command.operationRevision", "operation revision wire identity"),
    ("command.nativeTimerBindingId", "native binding wire identity"),
    ("command.expectedBindingRevision", "binding revision wire identity"),
    ("backendAgentNativeTimerDeleteFingerprintTokenValid(", "strict SHA-256 Timer fingerprint wire fence"),
    ("command.expectedNativeTimerFingerprint", "native Timer fingerprint wire token"),
    ("command.timerAssignmentId", "assignment wire identity"),
    ("command.backendNativeTimerId", "backend native Timer wire identity"),
    ("command.jobId", "job wire identity"),
    ("command.attemptId", "attempt wire identity"),
    ("command.claimEpoch", "claim epoch wire identity"),
    ("command.backendGeneration", "backend generation wire identity"),
    ("selection.ownershipGeneration", "provider ownership generation wire fence"),
    ("selection.providerInstanceEpoch", "provider instance epoch wire fence"),
    ("selection.providerGeneration", "provider generation wire fence"),
    ("selection.capabilityRevision", "provider capability revision wire fence"),
    ("request.localStartingPersistedAt", "durable starting wire evidence"),
):
    require(transport_source, needle, label)

require(
    transport_source,
    "BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect",
    "definitive disabled rejection mapping",
)
require(
    transport_source,
    "BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown",
    "ambiguous transport mapping",
)
forbid(
    transport_source,
    "BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified",
    "accepted Timer-delete outcome in disabled Agent transport",
)
for token in ('"DELT"', "cTimers", "Timers->", "/timers", "RESTfulAPI", "restfulapi"):
    forbid(transport_source, token, "direct VDR Timer mutation coupling")

require(plugin_header, "SuiteBridgeNativeTimerDeleteService", "plugin typed service")
require(plugin_header, "expectedNativeTimerFingerprint", "typed native Timer fingerprint request field")
require(plugin_source, 'strcasecmp(command, "NTDEL")', "private NTDEL command family")
require(plugin_source, 'values.front() == "CAP"', "NTDEL capability path")
require(plugin_source, 'values.front() == "EXEC"', "NTDEL execute path")
require(plugin_source, 'values.size() != 30', "fingerprint-aware typed execute shape")
require(plugin_source, "request.expectedNativeTimerFingerprint = values[10]", "native Timer fingerprint parser")
require(plugin_source, "safeFingerprintToken(request.expectedNativeTimerFingerprint)", "strict SHA-256 native Timer fingerprint parser")
require(plugin_source, "appendCanonical(canonical, request.expectedNativeTimerFingerprint)", "native Timer fingerprint replay identity")
require(plugin_source, '"ntdel:disabled:"', "disabled typed result")
require(plugin_source, "request.authorityDomain == AuthorityDomain", "authority fence")
require(plugin_source, "request.providerId == ProviderId", "provider id fence")
require(plugin_source, "request.providerInstanceEpoch == pluginInstanceEpoch_", "plugin epoch fence")
require(plugin_source, "request.providerGeneration == ProviderGeneration", "provider generation fence")
require(plugin_source, "request.capabilityRevision == CapabilityRevision", "capability revision fence")
require(plugin_source, "request.localStartingPersistedAt >= request.controlPlaneClaimedAt", "durable starting timestamp fence")

for token in (
    "<vdr/timers.h>",
    "cTimers",
    "Timers->",
    '"DELT"',
    "RESTfulAPI",
    "restfulapi",
):
    forbid(plugin_source, token, "direct VDR mutation in SuiteBridge transport service")

require(plugin_main_header, "SuiteBridgeNativeTimerDeleteService nativeTimerDelete_", "plugin service owner")
require(plugin_main, "nativeTimerDelete_(nativeProbe_.PluginInstanceEpoch())", "production callback remains unconfigured")
forbid(plugin_main, "ISuiteBridgeNativeTimerDeleteMutationCallback", "production mutation callback wiring")
require(plugin_main, "mutations=disabled execution=disabled", "truthful production disabled log")
require(plugin_svdrp, "nativeTimerDelete_.Handle(Command, Option)", "SVDRP typed handler wiring")
help_section = plugin_svdrp.split("SVDRPHelpPages", 1)[1].split("SVDRPCommand", 1)[0]
forbid(help_section, "NTDEL", "public SVDRP help advertisement")
require(plugin_makefile, "suitebridge_native_timer_delete.o", "plugin Timer-delete object build")
require(plugin_makefile, "test-native-timer-delete", "plugin disabled Timer-delete unit target")

forbid(agent_client, "SuiteBridgeNativeTimerDeleteTransport", "production Timer-delete adapter construction")
forbid(agent_client, "nativeTimerDeleteTransport", "production Timer-delete transport injection")
forbid(agent_client, "vdr.timer.delete", "production Timer-delete Agent configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete configuration")
available = command_client.split("CommandAvailability availableCommands(", 1)[1].split(
    "\n}\n}\n\nbool reconcileBackendAgentCommandState(", 1
)[0]
require(available, "kBackendAgentNativeTimerDeleteCommandType", "Timer-delete advertisement fence")
require(available, "continue;", "Timer-delete advertisement suppression")

for needle, label in (
    ("SuiteBridgeNativeTimerDeleteTransport transport", "dedicated adapter regression"),
    ('server.request() == "PLUG suitebridge NTDEL CAP 1\\r\\n"', "agent capability wire regression"),
    ("std::string fingerprintToken", "agent SHA-256 Timer fingerprint fixture"),
    ("request.command.expectedNativeTimerFingerprint == fingerprintToken()", "agent SHA-256 Timer fingerprint regression"),
    ("request.command.expectedNativeTimerFingerprint +", "agent native Timer fingerprint wire regression"),
    ("expectedNativeTimerFingerprint.clear()", "agent missing fingerprint rejection regression"),
    ("expectedNativeTimerFingerprint.back() = 'g'", "agent malformed digest rejection regression"),
    ("rejectedWithoutEffect", "agent disabled rejection regression"),
    ("outcomeUnknown", "agent transport ambiguity regression"),
    ("bad revision", "agent local wire validation regression"),
):
    require(agent_test, needle, label)
for needle, label in (
    ('service.Handle("NTDEL", "CAP 1")', "plugin capability regression"),
    ("lastExpectedNativeTimerFingerprint == fingerprintToken()", "plugin callback fingerprint regression"),
    ("conflictingNativeTimerFingerprint", "plugin changed native Timer fingerprint replay conflict"),
    ("malformedDigest", "plugin malformed SHA-256 fingerprint regression"),
    ("reply.replyCode == 556", "plugin disabled rejection regression"),
    ("reply.replyCode == 555", "plugin stale fence regression"),
    ("reply.replyCode == 501", "plugin malformed request regression"),
):
    require(plugin_test, needle, label)

require(mk, "$(AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC)", "dedicated adapter test source")
require(mk, "test-phase64-timer-delete-durable-executor-outcome", "Slice 33 regression dependency")
require(doc, "No real VDR Timer deletion", "no-mutation documentation")
require(doc, "replay ledger", "deferred replay-ledger boundary")
require(doc, "not advertised", "disabled Agent advertisement documentation")
require(doc, "separate adapter", "source-set layering documentation")

print("Phase 64 SuiteBridge native Timer-delete disabled transport architecture guard passed")
