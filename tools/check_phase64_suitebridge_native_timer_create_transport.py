#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required CREATE transport file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
sources = read("mk/agent-sources.mk")
svdrp_header = read(
    "core/agent/include/SuiteBridgeSvdrpTransport.h"
)
adapter_header = read(
    "core/agent/include/SuiteBridgeNativeTimerCreateTransport.h"
)
transport = read(
    "core/agent/src/SuiteBridgeSvdrpNativeTimerCreateTransport.cpp"
)
test = read(
    "core/agent/tests/test_suite_bridge_svdrp_native_timer_create_transport.cpp"
)
mk = read(
    "mk/phase64-suitebridge-native-timer-create-transport-tests.mk"
)
client = read(
    "core/agent/src/BackendAgentCommandClient.cpp"
)
agent_client = read(
    "core/agent/src/BackendAgentClient.cpp"
)
agent_main = read("apps/agent/main.cpp")
packaged = read(
    "packaging/systemd/backend-agent.conf"
)
executor_guard = read(
    "tools/check_phase64_native_timer_create_executor.py"
)

include = (
    "include "
    "mk/phase64-suitebridge-native-timer-create-transport-tests.mk"
)
require(makefile, include, "CREATE transport Make include")

if makefile.count(include) != 1:
    raise SystemExit(
        "CREATE transport Make include must occur exactly once"
    )

source_path = (
    "core/agent/src/"
    "SuiteBridgeSvdrpNativeTimerCreateTransport.cpp"
)

require(
    sources,
    "AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC :=",
    "dedicated CREATE transport source set",
)

if sources.count(source_path) != 1:
    raise SystemExit(
        "CREATE transport source must occur exactly once"
    )

create_block = sources.split(
    "AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC :=", 1
)[1].split(
    "\n\nAGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC", 1
)[0]

require(
    create_block,
    source_path,
    "dedicated CREATE transport ownership",
)

command_client_block = sources.split(
    "AGENT_COMMAND_CLIENT_SRC :=", 1
)[1].split(
    "\n\nAGENT_CONTROL_PLANE_DOMAIN_SRC", 1
)[0]

forbid(
    command_client_block,
    "AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC",
    "CREATE transport in installed CommandClient source set",
)

forbid(
    svdrp_header,
    "public IBackendAgentNativeTimerCreateTransport",
    "CREATE interface on generic SVDRP transport",
)

require(
    svdrp_header,
    "SuiteBridgeCommandReply discoverNativeTimerCreateContract();",
    "typed CREATE capability hook",
)

require(
    svdrp_header,
    "SuiteBridgeCommandReply executeNativeTimerCreateContract(",
    "typed CREATE execute hook",
)

require(
    adapter_header,
    "public IBackendAgentNativeTimerCreateTransport",
    "dedicated CREATE adapter interface",
)

require(
    adapter_header,
    "SuiteBridgeSvdrpTransport transport_;",
    "composed SVDRP transport",
)

for needle, label in (
    (
        '"PLUG suitebridge NTCREATE CAP 1\\r\\n"',
        "typed CREATE capability wire",
    ),
    (
        '"PLUG suitebridge NTCREATE EXEC vdr-suite-native/1 "',
        "typed CREATE execute wire",
    ),
    (
        "hexToken(command.expectedSpecificationFingerprint)",
        "specification fingerprint wire encoding",
    ),
    (
        "hexToken(specification.title)",
        "title whitespace-safe wire encoding",
    ),
    (
        "hexToken(specification.directory)",
        "directory whitespace-safe wire encoding",
    ),
    (
        "selection.ownershipGeneration",
        "provider ownership fence",
    ),
    (
        "selection.providerInstanceEpoch",
        "provider epoch fence",
    ),
    (
        "selection.providerGeneration",
        "provider generation fence",
    ),
    (
        "selection.capabilityRevision",
        "capability revision fence",
    ),
    (
        "request.localStartingPersistedAt",
        "durable starting evidence",
    ),
    (
        "BackendAgentNativeTimerCreateTransportDisposition::",
        "typed CREATE disposition mapping",
    ),
    (
        "acceptedUnverified",
        "accepted-unverified mapping",
    ),
    (
        "outcomeUnknown",
        "outcome-unknown mapping",
    ),
    (
        "rejectedWithoutEffect",
        "no-effect mapping",
    ),
):
    require(transport, needle, label)

for token in (
    "<vdr/timers.h>",
    "cTimers",
    "Timers->",
    '"NEWT"',
    '"MODT"',
    "RESTfulAPI",
    "restfulapi",
    "system(",
    "popen(",
):
    forbid(
        transport,
        token,
        "direct VDR Timer mutation coupling in Agent CREATE transport",
    )

for needle, label in (
    (
        "Tagesschau 20 Uhr",
        "whitespace-bearing title regression",
    ),
    (
        'server.request().find(\n            "Tagesschau 20 Uhr")',
        "raw title absence assertion",
    ),
    (
        "acceptedUnverified",
        "accepted transport regression",
    ),
    (
        "suitebridge:ntcreate:reply-fence-mismatch",
        "post-dispatch fence mismatch regression",
    ),
    (
        "changed after fingerprint",
        "specification fingerprint mismatch regression",
    ),
):
    require(test, needle, label)

require(
    mk,
    "$(AGENT_NATIVE_TIMER_CREATE_TRANSPORT_SRC)",
    "dedicated CREATE transport test source",
)

require(
    mk,
    "test-phase64-native-timer-create-executor",
    "CREATE executor regression dependency",
)

require(
    executor_guard,
    "SuiteBridge CREATE transport requires bounded successor guard",
    "executor successor-awareness",
)

availability_start = client.find(
    "CommandAvailability availableCommands"
)
availability_end = client.find(
    "bool reconcileBackendAgentCommandState",
    availability_start,
)

if availability_start < 0 or availability_end < 0:
    raise SystemExit(
        "CREATE advertisement function boundary not found"
    )

availability = client[
    availability_start:availability_end
]

require(
    availability,
    "kBackendAgentNativeTimerCreateCommandType",
    "CREATE advertisement fence",
)

require(
    availability,
    "discoverProvider",
    "CREATE provider discovery",
)
require(availability, "facts.available", "CREATE availability fence")

require(
    agent_main,
    "SuiteBridgeNativeTimerCreateTransport",
    "production CREATE adapter construction successor",
)

require(
    agent_client,
    "config_.nativeTimerCreateTransport",
    "production CREATE transport injection successor",
)

require(
    agent_client,
    "kBackendAgentNativeTimerCreateCommandType",
    "production CREATE Agent configuration allowlist",
)

require(
    packaged,
    "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,vdr.timer.toggle,vdr.timer.delete",
    "accepted packaged Timer activation",
)

# The private command-service successor may now own the parser/replay layer.
for plugin_path in (
    "vdr-plugin-suite-bridge/suitebridge_native_timer_create.h",
    "vdr-plugin-suite-bridge/suitebridge_native_timer_create.cpp",
):
    if not (ROOT / plugin_path).is_file():
        raise SystemExit(
            "missing CREATE command-service successor: " + plugin_path
        )

if not (
    ROOT
    / "vdr-plugin-suite-bridge/suitebridge_native_timer_create_vdr.cpp"
).is_file():
    raise SystemExit("missing CREATE VDR mutation successor")

plugin_svdrp = read(
    "vdr-plugin-suite-bridge/suitebridge_svdrp.cpp"
)

require(
    plugin_svdrp,
    "nativeTimerCreate_.Handle(Command, Option)",
    "private CREATE command-service dispatch",
)

help_start = plugin_svdrp.find("SVDRPHelpPages")
command_start = plugin_svdrp.find("SVDRPCommand", help_start)
if help_start < 0 or command_start < 0:
    raise SystemExit("SuiteBridge help boundary not found")
forbid(
    plugin_svdrp[help_start:command_start],
    "NTCREATE",
    "public CREATE Help advertisement",
)

print(
    "Phase 64 private SuiteBridge native Timer CREATE "
    "transport architecture guard passed"
)
