#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

files = [
    "core/vdr/include/VdrManagedTimerCreateReadbackEvidenceBuilder.h",
    "core/vdr/src/VdrManagedTimerCreateReadbackEvidenceBuilder.cpp",
    "core/vdr/tests/test_vdr_managed_timer_create_readback_evidence_builder.cpp",
    "docs/development/phase-64-vdr-managed-timer-create-readback-evidence.md",
    "mk/phase64-vdr-managed-timer-create-readback-evidence-tests.mk",
    "Makefile",
]
for relative in files:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing VDR managed CREATE readback evidence file: {relative}")

header = (ROOT / files[0]).read_text(encoding="utf-8")
source = (ROOT / files[1]).read_text(encoding="utf-8")
test = (ROOT / files[2]).read_text(encoding="utf-8")
doc = (ROOT / files[3]).read_text(encoding="utf-8")
mk = (ROOT / files[4]).read_text(encoding="utf-8")
makefile = (ROOT / files[5]).read_text(encoding="utf-8")

for token in [
    "VdrManagedTimerCreateReadbackEvidenceBuildStatus",
    "duplicateNativeTimerIdentity",
    "inventoryMismatch",
    "malformedManagedCorrelation",
    "conflictingManagedCorrelation",
    "class VdrManagedTimerCreateReadbackEvidenceBuilder",
]:
    if token not in header:
        raise SystemExit(f"missing VDR managed CREATE evidence header marker: {token}")

for token in [
    "nativeTimerInventoryEvidenceValid(inventory)",
    "VdrNativeTimerObservationMapper::map(",
    "parseVdrTimerManagedCorrelation(timer.aux)",
    "candidate.timerAssignmentId",
    "candidate.nativeTimerBindingId",
    "observedIds != inventory.backendNativeTimerIds",
    "nativeTimerCreateReadbackEvidenceValid(evidence)",
]:
    if token not in source:
        raise SystemExit(f"missing VDR managed CREATE evidence source marker: {token}")

for forbidden in [
    "IHttpClient", "RestfulApi", "SuiteBridge", "BackendAgentCommand", "SVDRP",
    "repository_", "MutationOperation",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature VDR managed CREATE evidence boundary crossing: {forbidden}")

for token in [
    "ambiguousEvidence.ok()",
    "duplicateNativeTimerIdentity",
    "inventoryMismatch",
    "invalidTimerObservation",
    "malformedManagedCorrelation",
    "conflictingManagedCorrelation",
    "test_vdr_managed_timer_create_readback_evidence_builder passed",
]:
    if token not in test:
        raise SystemExit(f"missing VDR managed CREATE evidence regression marker: {token}")

for token in [
    "complete native Timer inventory evidence",
    "opaque provider-local data",
    "exact native-id set",
    "does not collapse duplicate managed correlations",
]:
    if token not in doc:
        raise SystemExit(f"missing VDR managed CREATE evidence documentation marker: {token}")

include_line = "include mk/phase64-vdr-managed-timer-create-readback-evidence-tests.mk"
if include_line not in makefile:
    raise SystemExit("VDR managed CREATE evidence make fragment is not included")

for token in [
    "test-phase64-vdr-managed-timer-create-readback-evidence-architecture",
    "test-phase64-vdr-managed-timer-create-readback-evidence:",
    "test-fast: test-phase64-vdr-managed-timer-create-readback-evidence",
    "test-architecture: test-phase64-vdr-managed-timer-create-readback-evidence-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing VDR managed CREATE evidence make marker: {token}")

print("Phase-64 VDR managed Timer CREATE readback evidence check passed")
