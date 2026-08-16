#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 29 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
extension_header = read("core/agent/include/BackendAgentCommandStateExtension.h")
extension_source = read("core/agent/src/BackendAgentCommandStateExtension.cpp")
local_state_source = read("core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp")
timer_delete_source = read("core/agent/src/BackendAgentNativeTimerDelete.cpp")
agent_sources = read("mk/agent-sources.mk")
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")
test = read("core/agent/tests/test_backend_agent_command_state_v3.cpp")
mk = read("mk/phase64-command-state-v3-extension-tests.mk")
doc = read("docs/development/phase-64-command-state-v3-extension.md")

state_store_path = ROOT / "core/agent/src/BackendAgentCommandStateStore.cpp"
state_store = (
    state_store_path.read_text(encoding="utf-8")
    if state_store_path.is_file()
    else ""
)
state_owner = state_store if state_store else command_client

include = "include mk/phase64-command-state-v3-extension-tests.mk"
require(makefile, include, "Slice 29 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 29 Make include must occur exactly once")

# commands.state stays one exact-key owner with explicit version schemas. A
# bounded successor may extract that owner from CommandClient, but it must move
# the complete schema and durability boundary rather than split ownership.
require(state_owner, 'values["version"] == "1"', "v1 loader")
require(state_owner, 'values["version"] == "2"', "v2 loader")
require(state_owner, 'values["version"] == "3"', "v3 loader")
require(state_owner, "legacyKeys()", "v1 exact-key schema")
require(state_owner, "currentKeys()", "v2 exact-key schema")
require(state_owner, "extendedKeys()", "v3 exact-key schema")
require(state_owner, "exactKeys(values, expectedKeys)", "exact-key loader")
require(
    state_owner,
    'value.push_back("state_extension")',
    "single generic v3 extension key",
)
if state_owner.count('<< "state_extension="') != 1:
    raise SystemExit("commands.state must persist exactly one generic extension field")
require(state_owner, 'output << "version=3\\n"', "v3 persistence writer")

# The state owner delegates envelope and typed payload authority checks rather
# than learning mutation-specific fields.
require(
    state_owner,
    "BackendAgentCommandStateExtension",
    "generic extension state member",
)
require(
    state_owner,
    "backendAgentCommandStateExtensionParse",
    "generic extension envelope parse",
)
require(
    state_owner,
    "backendAgentCommandStateExtensionValidateSupported",
    "supported typed extension validation",
)
require(
    extension_header,
    "backendAgentCommandStateExtensionValidateSupported",
    "generic typed-validation API",
)
require(
    extension_source,
    "unsupported_command_state_extension_type",
    "unknown extension fail-closed fence",
)
require(
    extension_source,
    "backendAgentNativeTimerDeleteParseLocalState",
    "typed Timer-delete local-state decode",
)
require(
    extension_source,
    "backendAgentNativeTimerDeleteCommandFromAssignment",
    "typed assignment correlation",
)
forbid(command_client, "timer_delete_", "ad-hoc Timer-delete command-state field")

# Only the validation/persistence dependency chain joins the command-state
# source set. Bounded successor execution contracts must live in a distinct
# source set and must not weaken this state-owner boundary.
require(agent_sources, "AGENT_COMMAND_STATE_SRC :=", "bounded command-state source set")
for source in (
    "core/agent/src/BackendAgentNativeTimerDelete.cpp",
    "core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp",
    "core/agent/src/BackendAgentCommandStateExtension.cpp",
):
    if agent_sources.count(source) != 1:
        raise SystemExit(f"command-state runtime source must occur exactly once: {source}")
state_block = agent_sources.split("AGENT_COMMAND_STATE_SRC :=", 1)[1].split(
    "\n\nAGENT_TIMER_DELETE_EXECUTOR_SRC", 1
)[0] if "AGENT_TIMER_DELETE_EXECUTOR_SRC :=" in agent_sources else agent_sources.split(
    "AGENT_COMMAND_STATE_SRC :=", 1
)[1].split("\n\nAGENT_COMMAND_CLIENT_SRC", 1)[0]
for token in ("SuiteBridge", "Svdrp", "REST", "restful", "Executor", "Transport"):
    forbid(state_block, token, "mutation transport in command-state source set")

if state_store:
    state_store_source = "core/agent/src/BackendAgentCommandStateStore.cpp"
    if agent_sources.count(state_store_source) != 1:
        raise SystemExit("extracted command state store must occur exactly once")
    require(state_block, state_store_source, "extracted state store source-set ownership")
    require(
        command_client,
        '#include "BackendAgentCommandStateStore.h"',
        "CommandClient state-store dependency",
    )
    require(
        state_store,
        "namespace vdrsuite::agent::commandstate",
        "bounded state-store namespace",
    )
    for token in (
        "O_NOFOLLOW",
        "writeAll(",
        "fsync(",
        "rename(",
        "std::ifstream",
        "legacyKeys()",
        "extendedKeys()",
    ):
        forbid(command_client, token, "low-level commands.state ownership in CommandClient")

# Existing protected persistence is still the single durability boundary.
for token, label in (
    ("O_NOFOLLOW", "protected temporary open"),
    ("0600", "0600 file mode"),
    ("writeAll(descriptor, value)", "complete write"),
    ("fsync(descriptor)", "file fsync"),
    ("rename(temporary.c_str(), path.c_str())", "atomic rename"),
    ("syncParent(path)", "parent directory fsync"),
):
    require(state_owner, token, label)

# Timer-delete remains unavailable to the installed runtime despite the ability
# to preserve and validate its future local state.
require(
    command_client,
    "kBackendAgentNativeTimerDeleteCommandType",
    "explicit Timer-delete availability fence",
)
require(
    command_client,
    "continue;",
    "Timer-delete availability suppression",
)
forbid(agent_client, "vdr.timer.delete", "Timer-delete Agent configuration")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete configuration")

runtime_state_sources = "\n".join(
    (extension_source, local_state_source, timer_delete_source, state_store, command_client)
)
for token in (
    "SuiteBridgeSvdrp",
    "ISuiteBridgeLocalTransport",
    "RESTfulAPI",
    "restfulapi",
    "SVDRP",
    '"DELT"',
    "system(",
    "popen(",
    "curl ",
):
    forbid(runtime_state_sources, token, "Timer-delete mutation transport/shell coupling")

agent_src_dir = ROOT / "core/agent/src"
allowed_timer_delete_sources = {
    "BackendAgentNativeTimerDelete.cpp",
    "BackendAgentNativeTimerDeleteAssignment.cpp",
    "BackendAgentNativeTimerDeleteLocalState.cpp",
}
executor_source = agent_src_dir / "BackendAgentNativeTimerDeleteExecutor.cpp"
if executor_source.is_file():
    successor_guard = ROOT / "tools/check_phase64_timer_delete_fenced_executor.py"
    if not successor_guard.is_file():
        raise SystemExit("Timer-delete executor source requires bounded Slice 32 guard")
    require(
        agent_sources,
        "AGENT_TIMER_DELETE_EXECUTOR_SRC :=",
        "separate Timer-delete executor source set",
    )
    executor_tail = agent_sources.split(
        "AGENT_TIMER_DELETE_EXECUTOR_SRC :=", 1
    )[1]
    executor_block = (
        executor_tail.split("\n\nAGENT_NATIVE_PROBE_COMMAND_HANDLER_SRC", 1)[0]
        if "AGENT_NATIVE_PROBE_COMMAND_HANDLER_SRC :=" in executor_tail
        else executor_tail.split("\n\nAGENT_COMMAND_CLIENT_SRC", 1)[0]
    )
    if executor_block.count(
            "core/agent/src/BackendAgentNativeTimerDeleteExecutor.cpp") != 1:
        raise SystemExit(
            "bounded Timer-delete executor source must occur exactly once in its source set"
        )
    for token in ("SuiteBridge", "Svdrp", "REST", "restful"):
        forbid(executor_block, token, "concrete transport in executor source set")
    allowed_timer_delete_sources.add("BackendAgentNativeTimerDeleteExecutor.cpp")

# A later behavior-preserving modularization may extract orchestration into a
# dedicated command handler. That handler is allowed only as a separate source
# owner: commands.state parsing/durability stays in StateStore and concrete VDR
# or control-plane transport must not leak into the handler.
handler_source = agent_src_dir / "BackendAgentNativeTimerDeleteCommandHandler.cpp"
if handler_source.is_file():
    handler_guard = ROOT / "tools/check_phase64_timer_delete_durable_executor_outcome.py"
    if not handler_guard.is_file():
        raise SystemExit("Timer-delete command handler requires bounded Slice 33 guard")
    handler_text = handler_source.read_text(encoding="utf-8")
    require(
        agent_sources,
        "AGENT_NATIVE_TIMER_DELETE_COMMAND_HANDLER_SRC :=",
        "separate Timer-delete command handler source set",
    )
    handler_block = agent_sources.split(
        "AGENT_NATIVE_TIMER_DELETE_COMMAND_HANDLER_SRC :=", 1
    )[1].split("\n\nAGENT_COMMAND_CLIENT_SRC", 1)[0]
    handler_runtime_source = "core/agent/src/BackendAgentNativeTimerDeleteCommandHandler.cpp"
    if handler_block.count(handler_runtime_source) != 1 or agent_sources.count(handler_runtime_source) != 1:
        raise SystemExit("Timer-delete command handler source must occur exactly once in its source set")
    for token in (
        "SuiteBridgeSvdrp",
        "ISuiteBridgeLocalTransport",
        "RESTfulAPI",
        "restfulapi",
        '"DELT"',
        "/api/agent/v1/commands/poll",
        "/api/agent/v1/commands/receipt",
        "/api/agent/v1/commands/result",
        "IBackendAgentControlPlaneTransport",
    ):
        forbid(handler_text, token, "concrete/control-plane transport in Timer-delete command handler")
    for token in (
        "legacyKeys()",
        "extendedKeys()",
        "O_NOFOLLOW",
        "rename(",
        "fsync(",
    ):
        forbid(handler_text, token, "commands.state storage ownership in Timer-delete command handler")
    allowed_timer_delete_sources.add("BackendAgentNativeTimerDeleteCommandHandler.cpp")

# Slice 34 may add a concrete SuiteBridge Timer-delete transport only behind
# its own guard and source set. It must stay outside commands.state ownership
# and outside the installed CommandClient source graph while mutations remain
# disabled.
disabled_transport_source = agent_src_dir / "SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp"
if disabled_transport_source.is_file():
    transport_guard = ROOT / "tools/check_phase64_suitebridge_native_timer_delete_disabled_transport.py"
    if not transport_guard.is_file():
        raise SystemExit(
            "disabled SuiteBridge Timer-delete transport requires bounded Slice 34 guard"
        )
    transport_text = disabled_transport_source.read_text(encoding="utf-8")
    transport_runtime_source = "core/agent/src/SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp"
    require(
        agent_sources,
        "AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC :=",
        "separate disabled SuiteBridge Timer-delete transport source set",
    )
    transport_block = agent_sources.split(
        "AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC :=", 1
    )[1].split("\n\nAGENT_OBSERVATION_SRC", 1)[0]
    if transport_block.count(transport_runtime_source) != 1 or agent_sources.count(transport_runtime_source) != 1:
        raise SystemExit(
            "disabled SuiteBridge Timer-delete transport source must occur exactly once in its source set"
        )
    require(
        transport_text,
        "BackendAgentNativeTimerDeleteTransportDisposition::rejectedWithoutEffect",
        "disabled transport no-effect outcome",
    )
    require(
        transport_text,
        "BackendAgentNativeTimerDeleteTransportDisposition::outcomeUnknown",
        "disabled transport ambiguity outcome",
    )
    forbid(
        transport_text,
        "BackendAgentNativeTimerDeleteTransportDisposition::acceptedUnverified",
        "accepted outcome in disabled SuiteBridge Timer-delete transport",
    )
    for token in (
        "legacyKeys()",
        "extendedKeys()",
        "O_NOFOLLOW",
        "rename(",
        "fsync(",
        "/api/agent/v1/commands/poll",
        "/api/agent/v1/commands/receipt",
        "/api/agent/v1/commands/result",
        "IBackendAgentControlPlaneTransport",
    ):
        forbid(
            transport_text,
            token,
            "commands.state/control-plane ownership in disabled Timer-delete transport",
        )
    command_client_block = agent_sources.split("AGENT_COMMAND_CLIENT_SRC :=", 1)[1].split(
        "\n\nAGENT_CONTROL_PLANE_DOMAIN_SRC", 1
    )[0]
    forbid(
        command_client_block,
        "AGENT_NATIVE_TIMER_DELETE_TRANSPORT_SRC",
        "disabled concrete transport in installed CommandClient source set",
    )
    allowed_timer_delete_sources.add("SuiteBridgeSvdrpNativeTimerDeleteTransport.cpp")

for candidate in agent_src_dir.glob("*TimerDelete*.cpp"):
    if candidate.name not in allowed_timer_delete_sources:
        raise SystemExit(
            f"unexpected Timer-delete executor/transport source: {candidate.name}"
        )

# The focused regression suite covers every parser and persistence fence.
for needle, label in (
    ("expectAcceptedLoad(path, 1", "v1 load regression"),
    ("expectAcceptedLoad(path, 2", "v2 load regression"),
    ("expectAcceptedLoad(path, 3", "v3 empty-extension regression"),
    ("backendAgentNativeTimerDeleteParseCommandStateExtension", "typed v3 recovery"),
    ("Cross-command adoption is fail-closed", "cross-command rejection"),
    ("mismatching fingerprint inside cse1", "fingerprint rejection"),
    ("unknown mutation-bearing extension types", "unknown type rejection"),
    ('"cse1.malformed"', "malformed cse1 rejection"),
    ("40U * 1024U", "oversized extension rejection"),
    ("Newline/key injection", "newline/key injection rejection"),
    ("Duplicate keys remain rejected", "duplicate-key rejection"),
    ("assertTimerDeleteSuppressed", "Timer-delete advertisement regression"),
):
    require(test, needle, label)

require(
    mk,
    "test-phase63-command-delivery-runtime",
    "probe.noop Phase 63 regression dependency",
)
require(
    mk,
    "test-phase63-fenced-native-operation-runtime",
    "vdr.native.probe Phase 63 regression dependency",
)
require(doc, "No Timer-delete execution", "non-execution architecture boundary")
require(doc, "commands.state v3", "v3 state-owner documentation")

print("Phase 64 command state v3 extension architecture guard passed")
