#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing CREATE real-mutation file: {path}")
    return target.read_text(encoding="utf-8")

def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")

def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")

source = read("vdr-plugin-suite-bridge/suitebridge_native_timer_create_vdr.cpp")
header = read("vdr-plugin-suite-bridge/suitebridge_native_timer_create_vdr.h")
plugin = read("vdr-plugin-suite-bridge/suitebridge.h")
main = read("vdr-plugin-suite-bridge/suitebridge.cpp")
svdrp = read("vdr-plugin-suite-bridge/suitebridge_svdrp.cpp")
client = read("core/agent/src/BackendAgentCommandClient.cpp")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
agent_main = read("apps/agent/main.cpp")
packaged = read("packaging/systemd/backend-agent.conf")

for needle in (
    "ISuiteBridgeNativeTimerCreateMutationCallback", "CreateTimer(",
):
    require(header, needle, "typed CREATE callback")
for needle in (
    "#include <vdr/timers.h>",
    "cTimers::GetTimersWrite(stateKey, TimerWriteLockTimeoutMs)",
    "timer->Parse(definition.str().c_str())",
    "timers->SetExplicitModify()", "timers->Add(timer.get())",
    "timer.release()", "timers->SetModified()", "stateKey.Remove();",
    "AppliedUnverified", 'evidence("created-unverified"',
    '"<vdr-suite-managed-timer-v1 assignment=\\""',
    "hexIdentity(request.timerAssignmentId)",
    "hexIdentity(request.nativeTimerBindingId)",
    "<< file << ':' << aux",
):

    require(source, needle, "bounded CREATE VDR mutation")
if source.count("timers->Add(timer.get())") != 1:
    raise SystemExit("CREATE callback must contain exactly one native Add")
for token in ('"NEWT"', '"MODT"', "RESTfulAPI", "restfulapi",
              "system(", "popen(", "curl "):
    forbid(source, token, "fallback CREATE mutation")
require(plugin, "SuiteBridgeNativeTimerCreateVdrMutationCallback",
        "plugin callback owner")
require(main, "&nativeTimerCreateVdrMutation_", "production callback wiring")
require(svdrp, "nativeTimerCreate_.Handle(Command, Option)",
        "private NTCREATE dispatch")
help_section = svdrp.split("SVDRPHelpPages", 1)[1].split("SVDRPCommand", 1)[0]
forbid(help_section, "NTCREATE", "public CREATE Help advertisement")
available = client[
    client.find("CommandAvailability availableCommands"):
    client.find("bool reconcileBackendAgentCommandState")
]
require(available, "kBackendAgentNativeTimerCreateCommandType",
        "CREATE advertisement fence")
require(available, "continue;", "CREATE advertisement suppression")
require(agent_main, "SuiteBridgeNativeTimerCreateTransport",
        "production CREATE adapter construction successor")
require(agent_client, "config_.nativeTimerCreateTransport",
        "production CREATE transport injection successor")
forbid(packaged, "vdr.timer.create", "packaged CREATE advertisement")
print("Phase 64 SuiteBridge native Timer CREATE real-mutation guard passed")
