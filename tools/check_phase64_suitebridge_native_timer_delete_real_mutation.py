#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required real-mutation file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
mk = read("mk/phase64-suitebridge-native-timer-delete-real-mutation-tests.mk")
doc = read("docs/development/phase-64-suitebridge-native-timer-delete-real-mutation.md")
header = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete_vdr.h")
source = read("vdr-plugin-suite-bridge/suitebridge_native_timer_delete_vdr.cpp")
plugin_header = read("vdr-plugin-suite-bridge/suitebridge.h")
plugin_main = read("vdr-plugin-suite-bridge/suitebridge.cpp")
plugin_svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
plugin_makefile = read("vdr-plugin-suite-bridge/Makefile")
transport_source = read("core/agent/src/SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp")
transport_test = read("core/agent/tests/test_suite_bridge_svdrp_native_timer_delete_transport.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-suitebridge-native-timer-delete-real-mutation-tests.mk"
require(makefile, include, "real-mutation Make include")
if makefile.count(include) != 1:
    raise SystemExit("real-mutation Make include must occur exactly once")
require(mk, "test-phase64-suitebridge-native-timer-delete-real-mutation", "focused real-mutation test target")
require(mk, "test-phase64-suitebridge-native-timer-delete-replay-ledger-callback", "replay-ledger predecessor dependency")
require(mk, "test-fast: test-phase64-suitebridge-native-timer-delete-real-mutation", "fast regression wiring")

for needle, label in (
    ("SuiteBridgeNativeTimerDeleteVdrMutationCallback", "dedicated native VDR callback type"),
    ("ISuiteBridgeNativeTimerDeleteMutationCallback", "typed callback interface"),
    ("DeleteTimer(", "typed delete callback method"),
):
    require(header, needle, label)

for needle, label in (
    ('#include <vdr/timers.h>', "VDR Timer API include"),
    ("cTimers::GetTimersWrite(stateKey, TimerWriteLockTimeoutMs)", "bounded VDR Timer write lock"),
    ("timers->GetById(timerId, nullptr)", "local-only exact Timer lookup"),
    ("canonicalObservedState(*timer)", "live canonical Timer state"),
    ("sha256Token(canonical)", "live SHA-256 CAS token"),
    ("request.expectedNativeTimerFingerprint", "expected CAS token"),
    ("timer->Recording()", "recording safety fence"),
    ("timers->SetExplicitModify()", "explicit VDR modification marker"),
    ("timers->Del(timer)", "single native VDR Timer mutation"),
    ("timers->SetModified()", "VDR modified notification"),
    ("stateKey.Remove();", "committed VDR state-key release"),
    ("SuiteBridgeNativeTimerDeleteMutationDisposition::AppliedUnverified", "unverified local apply result"),
    ("SuiteBridgeNativeTimerDeleteMutationDisposition::RejectedWithoutEffect", "pre-effect rejection result"),
    ("SuiteBridgeNativeTimerDeleteMutationDisposition::OutcomeUnknown", "ambiguous exception result"),
    ('evidence("fingerprint-mismatch"', "CAS mismatch rejection evidence"),
    ('evidence("recording"', "recording rejection evidence"),
    ('evidence("not-found"', "missing Timer rejection evidence"),
    ('evidence("lock-unavailable"', "write-lock rejection evidence"),
    ('evidence("deleted-unverified"', "local deletion evidence"),
):
    require(source, needle, label)

if source.count("timers->Del(timer)") != 1:
    raise SystemExit("real Timer mutation must contain exactly one Timers::Del call")

ordered = [
    "cTimers::GetTimersWrite(stateKey, TimerWriteLockTimeoutMs)",
    "timers->GetById(timerId, nullptr)",
    "canonicalObservedState(*timer)",
    "liveFingerprint != request.expectedNativeTimerFingerprint",
    "timer->Recording()",
    "timers->Del(timer)",
]
positions = [source.find(token) for token in ordered]
if any(position < 0 for position in positions) or positions != sorted(positions):
    raise SystemExit(
        "real mutation ordering must be lock -> local lookup -> live canonical CAS -> recording fence -> delete"
    )

for token in (
    '"DELT"',
    "RESTfulAPI",
    "restfulapi",
    '"/timers',
    "system(",
    "popen(",
    "curl ",
    "timer->Skip",
    "cRecordControls::Process",
):
    forbid(source, token, "fallback/recording-stop mutation path")

require(
    source,
    'std::string fingerprint = "native-timer-observed-state/1|"',
    "canonical native Timer fingerprint schema",
)
for needle in (
    "appendField(fingerprint, *channel)",
    "appendField(fingerprint, eventId)",
    "appendField(fingerprint, title)",
    "appendField(fingerprint, directory)",
    "appendField(fingerprint, day(timer.Day()))",
    "appendField(fingerprint, weekdays(timer.WeekDays()))",
    "appendField(fingerprint, hhmm(timer.Start()))",
    "appendField(fingerprint, hhmm(timer.Stop()))",
    "appendInteger(fingerprint, timer.Flags())",
    "appendInteger(fingerprint, timer.Priority())",
    "appendInteger(fingerprint, timer.Lifetime())",
    "appendBoolean(fingerprint, (timer.Flags() & tfActive) != 0)",
    "appendBoolean(fingerprint, (timer.Flags() & tfVps) != 0)",
    "appendBoolean(fingerprint, timer.Recording())",
    "appendBoolean(fingerprint, timer.Pending())",
):
    require(source, needle, "canonical observed-state field")

require(plugin_header, "SuiteBridgeNativeTimerDeleteVdrMutationCallback nativeTimerDeleteVdrMutation_", "plugin callback owner")
require(plugin_main, "&nativeTimerDeleteVdrMutation_", "production callback wiring")
require(plugin_main, "mutations=enabled execution=enabled", "truthful mutation state log")
require(plugin_makefile, "suitebridge_native_timer_delete_vdr.o", "plugin real-mutation object build")
require(plugin_svdrp, "nativeTimerDelete_.Handle(Command, Option)", "private typed NTDEL handler")
help_section = plugin_svdrp.split("SVDRPHelpPages", 1)[1].split("SVDRPCommand", 1)[0]
forbid(help_section, "NTDEL", "public SVDRP Timer-delete help advertisement")

for needle, label in (
    ("native_timer_delete_suitebridge_provider_discovered_enabled", "enabled provider discovery"),
    ("BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified", "accepted-unverified Agent mapping"),
    ("AcceptedUnverifiedReplyCode", "accepted-unverified reply code"),
    ("OutcomeUnknownReplyCode", "outcome-unknown reply code"),
    ("reply-fence-mismatch", "post-dispatch provider fence mismatch"),
):
    require(transport_source, needle, label)
for needle, label in (
    ("native_timer_delete_suitebridge_provider_discovered_enabled", "enabled provider regression"),
    ("BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified", "accepted-unverified transport regression"),
    ("ntdel:vdr:deleted-unverified:cmd_1", "native delete evidence regression"),
):
    require(transport_test, needle, label)

# The real callback gate does not silently open the Agent/shipped command path.
forbid(agent_client, "SuiteBridgeNativeTimerDeleteTransport", "installed Agent Timer-delete transport construction")
forbid(agent_client, "nativeTimerDeleteTransport", "installed Agent Timer-delete transport injection")
forbid(agent_client, "vdr.timer.delete", "installed Agent Timer-delete configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete advertisement")
available = command_client.split("CommandAvailability availableCommands(", 1)[1].split(
    "\n}\n}\n\nbool reconcileBackendAgentCommandState(", 1
)[0]
require(available, "kBackendAgentNativeTimerDeleteCommandType", "Timer-delete advertisement fence")
require(available, "continue;", "Timer-delete advertisement suppression")

for needle, label in (
    ("first Phase-64 slice that contains and wires a real native VDR", "real-mutation scope documentation"),
    ("Lock-time fingerprint CAS", "lock-time CAS documentation"),
    ("currently recording is rejected", "recording safety documentation"),
    ("AppliedUnverified", "unverified mutation outcome documentation"),
    ("authoritative native-Timer absence readback", "authoritative readback requirement"),
    ("real yaVDR", "real-machine acceptance requirement"),
):
    require(doc, needle, label)

# This guard spans both lifecycle states of the destructive acceptance gate:
# before real-machine acceptance the document must explicitly say that no such
# acceptance is claimed; after acceptance it must carry explicit PASS evidence
# while preserving the shipped-Agent-closed boundary.
pre_acceptance = "No such real-machine acceptance is claimed"
post_acceptance = "Real yaVDR acceptance evidence"
if pre_acceptance not in doc and post_acceptance not in doc:
    raise SystemExit("missing explicit real-machine acceptance lifecycle state")
if post_acceptance in doc:
    require(doc, "### Acceptance boundary", "post-acceptance boundary documentation")
    require(
        doc,
        "The normal shipped Control-Plane -> Agent Timer-delete command path remained\nclosed throughout",
        "post-acceptance shipped Agent path remains closed",
    )
    require(
        doc,
        "destructive real-machine acceptance requirement for this native\nSuiteBridge/VDR mutation gate is satisfied",
        "post-acceptance gate completion documentation",
    )

print("Phase 64 SuiteBridge native VDR Timer-delete real-mutation architecture guard passed")
