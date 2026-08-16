#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        raise SystemExit(f"missing required Slice 28 file: {path}")
    return target.read_text(encoding="utf-8")


def require(text: str, needle: str, label: str) -> None:
    if needle not in text:
        raise SystemExit(f"missing {label}: {needle}")


def forbid(text: str, needle: str, label: str) -> None:
    if needle in text:
        raise SystemExit(f"forbidden {label}: {needle}")


makefile = read("Makefile")
header = read("core/agent/include/BackendAgentCommandStateExtension.h")
source = read("core/agent/src/BackendAgentCommandStateExtension.cpp")
test = read("core/agent/tests/test_backend_agent_command_state_extension.cpp")
create_test = read("core/agent/tests/test_backend_agent_native_timer_create_state_extension.cpp")
doc = read("docs/development/phase-64-command-state-extension.md")
mk = read("mk/phase64-command-state-extension-tests.mk")
agent_sources = read("mk/agent-sources.mk")
command_client = read("core/agent/src/BackendAgentCommandClient.cpp")
state_store_header_path = ROOT / "core/agent/include/BackendAgentCommandStateStore.h"
state_owner_header = (
    state_store_header_path.read_text(encoding="utf-8")
    if state_store_header_path.is_file()
    else command_client
)
agent_client = read("core/agent/src/BackendAgentClient.cpp")
packaged_config = read("packaging/systemd/backend-agent.conf")

include = "include mk/phase64-command-state-extension-tests.mk"
require(makefile, include, "Slice 28 Make include")
if makefile.count(include) != 1:
    raise SystemExit("Slice 28 Make include must occur exactly once")

require(header, "BackendAgentCommandStateExtension", "generic state extension envelope")
require(header, "vdr.timer.create.local-state.v1", "typed Timer-create extension type")
require(header, "vdr.timer.delete.local-state.v1", "typed Timer-delete extension type")
require(source, "extension.commandId != assignment.commandId", "command identity fence")
require(
    source,
    "extension.requestFingerprint != assignment.requestFingerprint",
    "request fingerprint fence",
)
require(source, "kMaximumExtensionPayloadBytes", "bounded extension payload")
require(source, 'std::string encoded = "cse1."', "single-line extension encoding")
require(
    source,
    "backendAgentNativeTimerCreateSerializeLocalState",
    "Phase 64 CREATE local-state embedding",
)
require(
    source,
    "backendAgentNativeTimerCreateParseLocalState",
    "strict Phase 64 CREATE local-state recovery",
)
for create_fence in (
    "left.expectedAssignmentRevision == right.expectedAssignmentRevision",
    "left.expectedIntentRevision == right.expectedIntentRevision",
    "left.assignmentEpoch == right.assignmentEpoch",
    "left.nativeTimerBindingId == right.nativeTimerBindingId",
    "left.expectedSpecificationFingerprint == right.expectedSpecificationFingerprint",
    "sameCreateSpecification(left.specification, right.specification)",
    "sameProviderSelection(left.localProviderSelection, right.localProviderSelection)",
):
    require(source, create_fence, "CREATE state-extension assignment fence")
require(
    source,
    "backendAgentNativeTimerDeleteSerializeLocalState",
    "Slice 27 state embedding",
)
require(
    source,
    "backendAgentNativeTimerDeleteParseLocalState",
    "strict Slice 27 state recovery",
)
require(test, "single-line safe value for commands.state", "commands.state embedding regression")
require(test, "different command identity", "cross-command adoption regression")
require(
    create_test,
    "Cross-command or cross-payload adoption must fail",
    "CREATE cross-assignment adoption regression",
)
require(
    create_test,
    "parsing the extension can never authorize a",
    "CREATE no-blind-retry extension regression",
)
require(
    create_test,
    "BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly",
    "CREATE recovered starting reconciliation-only regression",
)
require(doc, "Generic extension envelope", "generic extension architecture documentation")
require(doc, "No runtime wiring", "non-runtime boundary documentation")
require(mk, "test-phase64-command-state-extension", "focused Slice 28 target")
require(
    mk,
    "test_backend_agent_native_timer_create_state_extension",
    "CREATE state-extension focused binary",
)
require(
    mk,
    "core/agent/src/BackendAgentNativeTimerCreateRecovery.cpp",
    "CREATE conservative recovery link",
)

# Slice 28 defined the wrapper before runtime wiring. The v3 state-owner
# successor may now link it, and later Phase-64 CREATE work may add another
# typed payload, but this layer remains persistence/fencing only: it never
# advertises or executes native mutation.
require(
    agent_sources,
    "core/agent/src/BackendAgentCommandStateExtension.cpp",
    "generic state extension runtime validation wiring",
)
require(
    state_owner_header,
    '#include "BackendAgentCommandStateExtension.h"',
    "generic state owner extension integration",
)
forbid(command_client, '"vdr.timer.delete"', "Timer-delete command-client literal execution")
forbid(agent_client, '"vdr.timer.delete"', "Timer-delete Agent literal advertisement")
forbid(packaged_config, "vdr.timer.delete", "packaged Timer-delete advertisement")
forbid(packaged_config, "vdr.timer.create", "packaged Timer-create advertisement")

for token in (
    "SuiteBridgeSvdrp",
    "ISuiteBridgeLocalTransport",
    "restfulapi",
    "RESTfulAPI",
    "SVDRP",
    "system(",
    "popen(",
    "/timers",
    "createTimer(",
    "deleteTimer(",
):
    forbid(source, token, "native mutation/transport coupling in state extension")

print("Phase 64 command state extension architecture guard passed")
