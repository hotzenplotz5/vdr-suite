#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing CREATE command-service file: {path}")
    return target.read_text(encoding="utf-8")

def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")

def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")

header = read("vdr-plugin-suite-bridge/suitebridge_native_timer_create.h")
source = read("vdr-plugin-suite-bridge/suitebridge_native_timer_create.cpp")
test = read("vdr-plugin-suite-bridge/tests/test_suitebridge_native_timer_create.cpp")
plugin = read("vdr-plugin-suite-bridge/suitebridge.h")
svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
plugin_makefile = read("vdr-plugin-suite-bridge/Makefile")
client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")

for needle in (
    "timerAssignmentId", "expectedAssignmentRevision",
    "expectedIntentRevision", "assignmentEpoch", "nativeTimerBindingId",
    "expectedSpecificationFingerprint", "localStartingPersistedAt",
    "channelId", "title", "directory", "day", "weekdays",
    "startTime", "endTime", "priority", "lifetime", "enabled", "vps",
):
    require(header, needle, "CREATE request field")

for needle in (
    'strcasecmp(command, "NTCREATE")', 'values.front() == "CAP"',
    'values.front() == "EXEC"', "values.size() != 42",
    "hexValue(", "canonicalRequest(", "replayByOperationId_",
    '"replay_conflict"', '"in_progress"', '"ledger_full"',
    "localStartingPersistedAt >= request.controlPlaneClaimedAt",
):
    require(source, needle, "CREATE parser/replay/fence contract")

for token in (
    "<vdr/timers.h>", "cTimers", "Timers->", '"NEWT"', '"MODT"',
    "RESTfulAPI", "restfulapi", "system(", "popen(",
):
    forbid(source, token, "direct CREATE mutation")

require(test, "Tagesschau 20 Uhr", "decoded title regression")
require(plugin, "SuiteBridgeNativeTimerCreateService nativeTimerCreate_",
        "private CREATE service owner")
require(svdrp, "nativeTimerCreate_.Handle(Command, Option)",
        "private CREATE dispatch")
forbid(svdrp[svdrp.find("SVDRPHelpPages"):svdrp.find("SVDRPCommand")],
       "NTCREATE", "public CREATE help advertisement")
require(plugin_makefile, "suitebridge_native_timer_create.o",
        "CREATE service plugin object")
require(plugin_makefile, "test-native-timer-create",
        "CREATE plugin unit test")

if (ROOT / "vdr-plugin-suite-bridge/suitebridge_native_timer_create_vdr.cpp").exists():
    raise SystemExit("CREATE VDR mutation successor landed too early")

availability = client[
    client.find("CommandAvailability availableCommands"):
    client.find("bool reconcileBackendAgentCommandState")
]
require(availability, "kBackendAgentNativeTimerCreateCommandType",
        "CREATE advertisement fence")
require(availability, "continue;", "CREATE advertisement suppression")
forbid(agent_client, "SuiteBridgeNativeTimerCreateTransport",
       "production CREATE adapter construction")
forbid(agent_client, "nativeTimerCreateTransport",
       "production CREATE transport injection")

print("Phase 64 SuiteBridge NTCREATE command service architecture guard passed")
