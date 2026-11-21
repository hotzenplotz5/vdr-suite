#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required = [
    "core/agent/include/BackendAgentCommand.h",
    "core/agent/include/BackendAgentCommandDelivery.h",
    "core/agent/include/BackendAgentNativeProbe.h",
    "core/agent/src/BackendAgentCommandJson.cpp",
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    "core/agent/src/BackendAgentNativeProbeDelivery.cpp",
    "core/agent/src/BackendAgentCommandClient.cpp",
    "apps/tools/backend_agent_command_admin.cpp",
    "core/agent/tests/test_backend_agent_local_provider_selection_runtime.cpp",
    "docs/development/phase-63-local-provider-selection-runtime.md",
]
for relative in required:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing provider selection runtime file: {relative}")

command_header = (ROOT / required[0]).read_text(encoding="utf-8")
delivery_header = (ROOT / required[1]).read_text(encoding="utf-8")
native_header = (ROOT / required[2]).read_text(encoding="utf-8")
command_json = (ROOT / required[3]).read_text(encoding="utf-8")
delivery = (ROOT / required[4]).read_text(encoding="utf-8")
native_delivery = (ROOT / required[5]).read_text(encoding="utf-8")
client = (ROOT / required[6]).read_text(encoding="utf-8")
native_handler_path = ROOT / "core/agent/src/BackendAgentNativeProbeCommandHandler.cpp"
native_handler = (
    native_handler_path.read_text(encoding="utf-8")
    if native_handler_path.is_file()
    else client
)
state_store_path = ROOT / "core/agent/src/BackendAgentCommandStateStore.cpp"
state_store = (
    state_store_path.read_text(encoding="utf-8")
    if state_store_path.is_file()
    else client
)
admin = (ROOT / required[7]).read_text(encoding="utf-8")
test = (ROOT / required[8]).read_text(encoding="utf-8")
doc = (ROOT / required[9]).read_text(encoding="utf-8")

for token in [
    "localProviders",
    "BackendAgentLocalProviderFacts",
]:
    if token not in command_header:
        raise SystemExit(f"command poll missing provider fact token: {token}")

for token in [
    "selectLocalProvider",
    "localProviderSelectionCurrent",
    "setLocalProviderOwnership",
    "clearLocalProviderOwnership",
]:
    if token not in delivery_header:
        raise SystemExit(f"provider repository API missing: {token}")

for token in [
    "BackendAgentNativeProbePayload",
    "backendAgentNativeProbeParseSelectedPayload",
    "backendAgentNativeProbeSelectedPayload",
    "backendAgentNativeProbeProviderFacts",
    "backendAgentNativeProbeSelectionMatchesCapability",
]:
    if token not in native_header:
        raise SystemExit(f"selected native payload API missing: {token}")

for token in [
    '"localProviders"',
    '"providerInstanceEpoch"',
    '"providerGeneration"',
    '"capabilityRevision"',
    "backendAgentLocalProviderValidFacts",
]:
    if token not in command_json:
        raise SystemExit(f"provider poll JSON contract missing: {token}")

for token in [
    "backend_agent_local_provider_facts",
    "backend_agent_local_provider_ownership",
    "backend_agent_command_provider_selections",
    '"local_provider_ownership_required"',
    '"local_provider_selection_stale"',
    "o.active=1",
    "f.available=1",
]:
    if token not in delivery:
        raise SystemExit(f"provider persistence/fence missing: {token}")

receipt_start = delivery.find(
    "BackendAgentCommandReceiptResult BackendAgentCommandRepository::acceptReceipt"
)
result_start = delivery.find(
    "BackendAgentCommandResultAck BackendAgentCommandRepository::acceptResult"
)
replay_start = delivery.find(
    "bool BackendAgentCommandRepository::requestReplay", result_start
)
if min(receipt_start, result_start, replay_start) < 0 or not (
    receipt_start < result_start < replay_start
):
    raise SystemExit("unable to locate provider receipt/result fence sections")
receipt_section = delivery[receipt_start:result_start]
result_section = delivery[result_start:replay_start]

if "localProviderSelectionCurrent" not in receipt_section:
    raise SystemExit(
        "native receipt must cross the current provider-selection authority fence"
    )
if "localProviderSelectionCurrent" in result_section:
    raise SystemExit(
        "outcome result ingestion must not require current provider ownership"
    )
for token in [
    "backendAgentNativeProbeParseSelectedPayload",
    "backend_agent_command_provider_selections",
    "selection_identity",
    "backendAgentLocalProviderSelectionIdentity",
]:
    if token not in result_section:
        raise SystemExit(
            f"outcome evidence must remain bound to immutable selection identity: {token}"
        )

for token in [
    "selectLocalProvider(",
    '"vdr.native"',
    '"vdr.native.probe"',
    "assignment.payloadVersion = 2",
    "backendAgentNativeProbeSelectedPayload",
    "insertAssignment(assignment, &*selection)",
]:
    if token not in native_delivery:
        raise SystemExit(f"native assignment selection missing: {token}")

for token in [
    "CommandAvailability",
    "backendAgentNativeProbeCommandAvailability",
    "backendAgentNativeProbeCommandReconcile",
]:
    if token not in client:
        raise SystemExit(f"agent provider handoff missing: {token}")

for token in [
    "backendAgentNativeProbeProviderFacts",
    "backendAgentNativeProbeSelectionMatchesCapability",
    'state.assignment.payloadVersion != 2',
    '"native probe provider selection required"',
]:
    if token not in native_handler:
        raise SystemExit(f"agent provider fence missing: {token}")

if "state.receiptAcknowledged = false" not in state_store:
    raise SystemExit(
        "agent provider recovery fence missing: state.receiptAcknowledged = false"
    )

for token in [
    '"--provider-ownership-status"',
    '"--set-native-probe-owner"',
    '"--clear-native-probe-owner"',
    '"suitebridge:local"',
]:
    if token not in admin:
        raise SystemExit(f"explicit provider ownership admin missing: {token}")

for token in [
    "alternate provider is descriptive only",
    "local_provider_selection_stale",
    "ownershipGeneration==3",
    "capabilityRevision",
    "preservedResult.accepted",
    "legacyResult.accepted",
]:
    if token not in test:
        raise SystemExit(f"provider selection regression coverage missing: {token}")

for token in [
    "Availability is not authority.",
    "There is no API in this slice that chooses among a list of available providers.",
    "`BackendNode.online` remains untouched.",
    "newly received v1 `vdr.native.probe`",
    "Result evidence does not authorize",
    "legacy v1 durable receipt and result evidence",
    "bounded real-yaVDR acceptance",
]:
    if token not in doc:
        raise SystemExit(f"provider runtime contract statement missing: {token}")

backend_node = (ROOT / "core/vdr/include/BackendNode.h").read_text(encoding="utf-8")
factory = (ROOT / "core/vdr/src/VdrAdapterFactory.cpp").read_text(encoding="utf-8")
http_server = (ROOT / "core/agent/src/BackendAgentHttpServer.cpp").read_text(encoding="utf-8")
if "BackendAgentLocalProvider" in backend_node:
    raise SystemExit("provider availability leaked into BackendNode")
if "BackendAgentLocalProvider" in factory:
    raise SystemExit("provider ownership leaked into legacy VdrAdapterFactory")
for forbidden in ["/providers", "/provider-ownership", "/local-provider"]:
    if forbidden in http_server:
        raise SystemExit(f"public provider route introduced: {forbidden}")

print("PHASE_63_LOCAL_PROVIDER_SELECTION_RUNTIME=PASS")
