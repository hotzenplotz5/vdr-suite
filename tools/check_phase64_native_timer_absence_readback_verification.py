#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerAbsenceReadbackVerificationService.h"
SOURCE = ROOT / "core/timers/src/NativeTimerAbsenceReadbackVerificationService.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_absence_readback_verification_service.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-absence-readback-verification.md"
MK = ROOT / "mk/phase64-native-timer-absence-readback-verification-tests.mk"
MAKEFILE = ROOT / "Makefile"

for path in [HEADER, SOURCE, TEST, DOC, MK, MAKEFILE]:
    if not path.is_file():
        raise SystemExit(f"missing Slice-19 file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
source = SOURCE.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
mk = MK.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

for token in [
    "NativeTimerAbsenceReadbackVerificationStatus",
    "verified",
    "alreadyVerified",
    "bindingRevisionConflict",
    "identityConflict",
    "ownershipConflict",
    "generationConflict",
    "staleEvidence",
    "reconciliationRequired",
    "repositoryConflict",
    "class NativeTimerAbsenceReadbackVerificationService",
    "NativeTimerAbsenceReadbackExpectation",
    "NativeTimerInventoryEvidence",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-19 header marker: {token}")

for token in [
    "nativeTimerAbsenceReadbackExpectationValid(expectation)",
    "nativeTimerInventoryEvidenceValid(evidence)",
    "repository_.findById(expectation.nativeTimerBindingId)",
    "NativeTimerBindingOwnership::managed",
    "NativeTimerBindingOwnership::adopted",
    "current.backendGeneration > expectation.backendGeneration",
    "assessNativeTimerAbsence(evidence, request)",
    "NativeTimerAbsenceAssessmentStatus::present",
    "alreadyVerified(current, expectation)",
    "nativeTimerBindingRevisionMatches(",
    "evidence.observedAt < current.lastObservedAt",
    "next.lastVerifiedOperationId = expectation.operationId",
    "if (current.missingSince == 0)",
    "next.missingSince = evidence.observedAt",
    "next.driftState = NativeTimerBindingDriftState::expectedTransition",
    "repository_.update(next, current.bindingRevision)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-19 source marker: {token}")

for forbidden in [
    "sqlite3_",
    "VdrTimer",
    "RestfulApi",
    "NativeTimerBindingAbsenceApplicationService",
    "TimerAssignment",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
    "driftState = NativeTimerBindingDriftState::externalDelete",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature Slice-19 boundary crossing: {forbidden}")

for token in [
    "NativeTimerAbsenceReadbackVerificationStatus::verified",
    'verified.binding.lastVerifiedOperationId == "operation:delete:1"',
    "NativeTimerBindingDriftState::expectedTransition",
    "NativeTimerAbsenceReadbackVerificationStatus::alreadyVerified",
    "presentAfterVerified",
    "NativeTimerAbsenceReadbackVerificationStatus::reconciliationRequired",
    "NativeTimerAbsenceReadbackVerificationStatus::bindingRevisionConflict",
    "NativeTimerAbsenceReadbackVerificationStatus::staleEvidence",
    "NativeTimerAbsenceReadbackVerificationStatus::generationConflict",
    "NativeTimerAbsenceReadbackVerificationStatus::identityConflict",
    "NativeTimerAbsenceReadbackVerificationStatus::ownershipConflict",
    "priorVerified.binding.missingSince == 1800",
    "priorVerified.binding.driftState == NativeTimerBindingDriftState::ambiguous",
    "classifiedVerified.binding.driftState == NativeTimerBindingDriftState::externalDelete",
    "test_native_timer_absence_readback_verification_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-19 regression marker: {token}")

for token in [
    "Operation-aware Native Timer Absence Readback Verification",
    "postcondition",
    "verifies the expected postcondition",
    "Existing missing evidence is not rewritten",
    "Idempotent replay does not hide reappearance",
    "Slice-17 generic absence application",
    "no hidden retry",
    "operation repository",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-19 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-absence-readback-verification-architecture",
    "test-phase64-native-timer-absence-readback-verification:",
    "core/timers/src/NativeTimerAbsenceReadbackVerificationService.cpp",
    "core/timers/tests/test_native_timer_absence_readback_verification_service.cpp",
    "test-fast: test-phase64-native-timer-absence-readback-verification",
    "test-architecture: test-phase64-native-timer-absence-readback-verification-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-19 make marker: {token}")

if "include mk/phase64-native-timer-absence-readback-verification-tests.mk" not in makefile:
    raise SystemExit("Slice-19 make fragment is not included")

for scan_root in [
    ROOT / "apps", ROOT / "api", ROOT / "core" / "agent",
    ROOT / "core" / "daemon", ROOT / "core" / "http",
    ROOT / "core" / "runtime", ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc",
            ".mk", ".conf", ".service",
        }:
            continue
        if "NativeTimerAbsenceReadbackVerificationService" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature Slice-19 runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer absence readback verification check passed")
print(
    "Slice-19 boundary: operation-aware complete-inventory absence verification only; "
    "operation lifecycle orchestration and native mutation deferred"
)
