#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

header_path = "core/timers/include/NativeTimerPresentReadbackVerificationService.h"
source_path = "core/timers/src/NativeTimerPresentReadbackVerificationService.cpp"
test_path = "core/timers/tests/test_native_timer_present_readback_verification_service.cpp"
doc_path = "docs/development/phase-64-native-timer-present-readback-verification.md"
mk_path = "mk/phase64-native-timer-present-readback-verification-tests.mk"
makefile_path = "Makefile"

for relative in [
    "core/timers/include/NativeTimerBinding.h",
    "core/timers/include/NativeTimerObservation.h",
    "core/timers/include/NativeTimerReadbackExpectation.h",
    "core/timers/include/NativeTimerBindingRepository.h",
    header_path, source_path, test_path, doc_path, mk_path, makefile_path,
]:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Slice-14 present-verification file: {relative}")

header = (ROOT / header_path).read_text(encoding="utf-8")
source = (ROOT / source_path).read_text(encoding="utf-8")
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
mk = (ROOT / mk_path).read_text(encoding="utf-8")
makefile = (ROOT / makefile_path).read_text(encoding="utf-8")

for token in [
    "NativeTimerPresentReadbackVerificationStatus",
    "verified",
    "alreadyVerified",
    "bindingRevisionConflict",
    "identityConflict",
    "ownershipConflict",
    "generationConflict",
    "staleObservation",
    "reconciliationRequired",
    "repositoryConflict",
    "class NativeTimerPresentReadbackVerificationService",
    "NativeTimerReadbackExpectation",
    "NativeTimerObservation",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-14 header marker: {token}")

for token in [
    "nativeTimerReadbackExpectationValid(expectation)",
    "nativeTimerObservationValid(observation)",
    "repository_.findById(expectation.nativeTimerBindingId)",
    "observation.backendGeneration != expectation.backendGeneration",
    "current.backendGeneration > expectation.backendGeneration",
    "observation.observedAt < expectation.readbackNotBefore",
    "observation.observedFingerprint != expectation.expectedFingerprint",
    "alreadyVerified(current, expectation)",
    "nativeTimerBindingRevisionMatches(",
    "observation.observedAt < current.lastObservedAt",
    "next.observedState = observation.observedState",
    "next.lastVerifiedOperationId = expectation.operationId",
    "next.missingSince = 0",
    "next.driftState = NativeTimerBindingDriftState::none",
    "repository_.update(next, current.bindingRevision)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-14 source marker: {token}")

for forbidden in [
    "sqlite3_",
    "VdrTimer",
    "TimerAssignment",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
    "externalFieldChange",
    "externalDelete",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature Slice-14 boundary crossing: {forbidden}")

for token in [
    "NativeTimerPresentReadbackVerificationStatus::verified",
    'verified.binding.lastVerifiedOperationId == "operation:1"',
    "verified.binding.missingSince == 0",
    "verified.binding.driftState == NativeTimerBindingDriftState::none",
    "NativeTimerPresentReadbackVerificationStatus::alreadyVerified",
    "changedAfterVerified",
    "NativeTimerPresentReadbackVerificationStatus::reconciliationRequired",
    "NativeTimerPresentReadbackVerificationStatus::bindingRevisionConflict",
    "NativeTimerPresentReadbackVerificationStatus::staleObservation",
    "NativeTimerPresentReadbackVerificationStatus::generationConflict",
    "NativeTimerPresentReadbackVerificationStatus::identityConflict",
    "NativeTimerPresentReadbackVerificationStatus::ownershipConflict",
    "NativeTimerPresentReadbackVerificationStatus::bindingNotFound",
    "test_native_timer_present_readback_verification_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-14 regression marker: {token}")

for token in [
    "Operation-aware Native Timer Present Readback Verification",
    "`readbackNotBefore`",
    "current incoming observation",
    "`reconciliationRequired`",
    "`lastVerifiedOperationId`",
    "actual authoritative readback representation",
    "authoritative native Timer absence evidence",
    "no hidden retry",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-14 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-present-readback-verification-architecture",
    "test-phase64-native-timer-present-readback-verification:",
    "core/timers/src/NativeTimerPresentReadbackVerificationService.cpp",
    "core/timers/tests/test_native_timer_present_readback_verification_service.cpp",
    "test-fast: test-phase64-native-timer-present-readback-verification",
    "test-architecture: test-phase64-native-timer-present-readback-verification-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-14 make marker: {token}")

include_line = "include mk/phase64-native-timer-present-readback-verification-tests.mk"
if include_line not in makefile:
    raise SystemExit("Slice-14 make fragment is not included")

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
        if "NativeTimerPresentReadbackVerificationService" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature present-verification runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer present-readback verification check passed")
print(
    "Slice-14 boundary: exact operation-aware present verification only; "
    "absence, operation lifecycle orchestration and native mutation deferred"
)
