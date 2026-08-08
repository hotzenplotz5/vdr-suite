#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/agent/include/BackendAgentProtectedWrite.h",
    "core/agent/src/BackendAgentProtectedWrite.cpp",
    "core/agent/tests/test_backend_agent_protected_write.cpp",
    "docs/development/phase-63-protected-write-contract.md",
    "docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md",
    "docs/adr/ADR-0043-job-claim-retry-saga-execution-model.md",
    "mk/phase63-local-provider-tests.mk",
]
for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing protected-write contract file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
makefile = (ROOT / required_files[6]).read_text(encoding="utf-8")

for token in [
    "BackendAgentProtectedWriteResourceRef",
    "BackendAgentProtectedWriteLease",
    "BackendAgentProtectedWriteIdempotencyRecord",
    "BackendAgentProtectedWriteRequest",
    "expectedAuthorityGeneration",
    "requiredWriteCapability",
    "providerSelection",
    "readbackRequired",
    "reserveThenExecuteOnce",
    "returnPersistedResult",
    "reconcileUnknownOutcome",
]:
    if token not in header:
        raise SystemExit(f"missing protected-write contract token: {token}")

for token in [
    '"protected_write_idempotency_conflict"',
    '"protected_write_outcome_requires_reconciliation"',
    '"protected_write_lease_expired"',
    '"protected_write_lease_capability_not_authorized"',
    '"protected_write_lease_resource_not_authorized"',
    '"protected_write_authority_generation_changed"',
    '"protected_write_provider_selection_not_current"',
    '"protected_write_revision_conflict"',
    '"protected_write_resource_state_missing"',
    '"protected_write_authorized_once"',
    "backendAgentLocalProviderSelectionUsable",
]:
    if token not in source:
        raise SystemExit(f"missing protected-write fail-closed rule: {token}")

order = [
    source.find("if (idempotency.present)"),
    source.find("if (!backendAgentProtectedWriteValidLease(lease))"),
    source.find("request.expectedAuthorityGeneration != lease.authorityGeneration"),
    source.find("backendAgentLocalProviderSelectionUsable"),
    source.find("observed->currentRevision != resource.expectedRevision"),
    source.find("BackendAgentProtectedWriteDecision::reserveThenExecuteOnce"),
]
if min(order) < 0 or order != sorted(order):
    raise SystemExit(
        "protected-write decision order must be idempotency, lease, authority/provider, revision, reserve"
    )

for token in [
    "Same idempotency key with a changed logical request fails before lease checks.",
    "A reserved or unknown exact operation is reconciliation-only",
    "Authority generation is distinct from resource revision",
    "Protected writes always demand authoritative post-write readback.",
]:
    if token not in test:
        raise SystemExit(f"missing protected-write regression statement: {token}")

for token in [
    "ADR-0042",
    "ADR-0043",
    "Availability is not authority.",
    "durably reserve the idempotency key before native execution",
    "Unknown outcome is reconciliation-only.",
    "Expected authority generation and expected resource revision are separate fences.",
    "mutations=disabled",
    "No TimerIntent, RecordingIntent, SearchTimer, Remote, OSD, configuration or metadata mutation is introduced.",
    "no real yaVDR acceptance is required",
]:
    if token not in doc:
        raise SystemExit(f"missing protected-write boundary statement: {token}")

for token in [
    "test-phase63-protected-write-contract-architecture",
    "test-phase63-protected-write-contract",
    "core/agent/src/BackendAgentProtectedWrite.cpp",
    "core/agent/tests/test_backend_agent_protected_write.cpp",
]:
    if token not in makefile:
        raise SystemExit(f"protected-write test graph missing token: {token}")

for forbidden in [
    "sqlite3",
    "vdr.native.probe",
    "vdr.timers",
    "vdr.recordings",
    "mutations=enabled",
]:
    if forbidden in source:
        raise SystemExit(f"contract-only protected-write source contains runtime/domain token: {forbidden}")

runtime_files = [
    "apps/agent/main.cpp",
    "core/agent/src/BackendAgentCommandDelivery.cpp",
    "core/agent/src/BackendAgentNativeProbeDelivery.cpp",
    "core/vdr/src/VdrAdapterFactory.cpp",
]
for relative in runtime_files:
    text = (ROOT / relative).read_text(encoding="utf-8")
    if "BackendAgentProtectedWrite" in text:
        raise SystemExit(f"contract-only protected-write model wired into runtime: {relative}")

print("PHASE_63_PROTECTED_WRITE_CONTRACT=PASS")
