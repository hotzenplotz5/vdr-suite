#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/agent/include/BackendAgentLocalProvider.h",
    "core/agent/src/BackendAgentLocalProvider.cpp",
    "core/agent/tests/test_backend_agent_local_provider.cpp",
    "docs/development/phase-63-local-provider-ownership.md",
]
for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing local provider ownership contract file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")

for token in [
    "BackendAgentLocalProviderFacts",
    "BackendAgentLocalProviderOwnership",
    "BackendAgentLocalProviderSelection",
    "providerInstanceEpoch",
    "ownershipGeneration",
    "capabilityRevision",
    "requiredCapability",
]:
    if token not in header:
        raise SystemExit(f"missing provider contract token: {token}")

for token in [
    'providerKind == "suitebridge"',
    'providerKind == "restfulapi"',
    'providerKind == "external"',
    '"local_provider_not_owner"',
    '"local_provider_instance_epoch_changed"',
    '"local_provider_generation_changed"',
    '"local_provider_capability_revision_changed"',
]:
    if token not in source:
        raise SystemExit(f"missing fail-closed provider guard: {token}")

for token in [
    "Availability is not authority.",
    "There is no API in this slice that chooses among a list of available providers.",
    "BackendNode.online",
    "continuation candidate",
    "no real yaVDR acceptance is required",
]:
    if token not in doc:
        raise SystemExit(f"missing provider contract statement: {token}")

for relative in [
    "apps/agent/main.cpp",
    "core/agent/include/BackendAgentCommand.h",
    "core/agent/src/BackendAgentNativeProbeDelivery.cpp",
    "core/vdr/include/BackendNode.h",
    "core/vdr/src/VdrAdapterFactory.cpp",
]:
    text = (ROOT / relative).read_text(encoding="utf-8")
    if "BackendAgentLocalProvider" in text:
        raise SystemExit(f"contract-only provider model wired into runtime: {relative}")

print("PHASE_63_LOCAL_PROVIDER_OWNERSHIP_CONTRACT=PASS")
