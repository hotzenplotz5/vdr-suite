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

include = "include mk/phase64-command-state-v3-extension-tests.mk"
require(makefile, include, "Slice 29 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 29 Make include must occur exactly once")

# commands.state stays one exact-key owner with explicit version schemas.
require(command_client, 'values["version"] == "1"', "v1 loader")
require(command_client, 'values["version"] == "2"', "v2 loader")
require(command_client, 'values["version"] == "3"', "v3 loader")
require(command_client, "legacyKeys()", "v1 exact-key schema")
require(command_client, "currentKeys()", "v2 exact-key schema")
require(command_client, "extendedKeys()", "v3 exact-key schema")
require(command_client, "exactKeys(values, expectedKeys)", "exact-key loader")
require(
    command_client,
    'value.push_back("state_extension")',
    "single generic v3 extension key",
)
if command_client.count('<< "state_extension="') != 1:
    raise SystemExit("commands.state must persist exactly one generic extension field")
require(command_client, 'output << "version=3\\n"', "v3 persistence writer")

# The state owner delegates envelope and typed payload authority checks rather
# than learning mutation-specific fields.
require(
    command_client,
    "BackendAgentCommandStateExtension",
    "generic extension state member",
)
require(
    command_client,
    "backendAgentCommandStateExtensionParse",
    "generic extension envelope parse",
)
require(
    command_client,
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

# Only the validation/persistence dependency chain joins the Agent command
# client link path. No daemon or SuiteBridge mutation transport is added.
require(agent_sources, "AGENT_COMMAND_STATE_SRC :=", "bounded command-state source set")
for source in (
    "core/agent/src/BackendAgentNativeTimerDelete.cpp",
    "core/agent/src/BackendAgentNativeTimerDeleteLocalState.cpp",
    "core/agent/src/BackendAgentCommandStateExtension.cpp",
):
    if agent_sources.count(source) != 1:
        raise SystemExit(f"command-state runtime source must occur exactly once: {source}")
state_block = agent_sources.split("AGENT_COMMAND_STATE_SRC :=", 1)[1].split(
    "\n\nAGENT_COMMAND_CLIENT_SRC", 1
)[0]
for token in ("SuiteBridge", "Svdrp", "REST", "restful", "Executor", "Transport"):
    forbid(state_block, token, "mutation transport in command-state source set")

# Existing protected persistence is still the single durability boundary.
for token, label in (
    ("O_NOFOLLOW", "protected temporary open"),
    ("0600", "0600 file mode"),
    ("writeAll(descriptor, value)", "complete write"),
    ("fsync(descriptor)", "file fsync"),
    ("rename(temporary.c_str(), path.c_str())", "atomic rename"),
    ("syncParent(path)", "parent directory fsync"),
):
    require(command_client, token, label)

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
    (extension_source, local_state_source, timer_delete_source, command_client)
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
for candidate in agent_src_dir.glob("*TimerDelete*.cpp"):
    if candidate.name not in allowed_timer_delete_sources:
        raise SystemExit(
            f"unexpected Timer-delete executor/transport source: {candidate.name}"
        )

# The focused regression suite covers every new parser and persistence fence.
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
