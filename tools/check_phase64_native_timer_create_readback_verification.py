#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "header": "core/timers/include/NativeTimerCreateReadbackVerificationService.h",
    "source": "core/timers/src/NativeTimerCreateReadbackVerificationService.cpp",
    "test": "core/timers/tests/test_native_timer_create_readback_verification_service.cpp",
    "doc": "docs/development/phase-64-native-timer-create-readback-verification.md",
    "mk": "mk/phase64-native-timer-create-readback-verification-tests.mk",
    "makefile": "Makefile",
}

for relative in paths.values():
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Phase-64 CREATE readback verification file: {relative}")

header = (ROOT / paths["header"]).read_text(encoding="utf-8")
source = (ROOT / paths["source"]).read_text(encoding="utf-8")
test = (ROOT / paths["test"]).read_text(encoding="utf-8")
doc = (ROOT / paths["doc"]).read_text(encoding="utf-8")
mk = (ROOT / paths["mk"]).read_text(encoding="utf-8")
makefile = (ROOT / paths["makefile"]).read_text(encoding="utf-8")

for token in [
    "NativeTimerCreateReadbackVerificationStatus",
    "correlationNotFound",
    "correlationAmbiguous",
    "reconciliationRequired",
    "nativeIdentityConflict",
    "assignmentBindingConflict",
    "class NativeTimerCreateReadbackVerificationService",
]:
    if token not in header:
        raise SystemExit(f"missing CREATE verification header marker: {token}")

for token in [
    "nativeTimerCreateReadbackExpectationValid(expectation)",
    "nativeTimerCreateReadbackEvidenceValid(evidence)",
    "evidence.backendGeneration != expectation.backendGeneration",
    "evidence.observedAt < expectation.readbackNotBefore",
    "correlationMatches(candidate, expectation)",
    "nativeTimerObservationMatchesSpecification(",
    "repository_.findById(expectation.nativeTimerBindingId)",
    "binding.ownership = NativeTimerBindingOwnership::managed",
    "binding.lastVerifiedOperationId = expectation.operationId",
    "repository_.create(binding)",
]:
    if token not in source:
        raise SystemExit(f"missing CREATE verification source marker: {token}")

for forbidden in [
    "VdrTimer", "SuiteBridge", "BackendAgentCommand", "SVDRP", "sqlite3_",
    "RestfulApi", "TimerAssignmentRepository",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature CREATE verification boundary crossing: {forbidden}")

for token in [
    "NativeTimerCreateReadbackVerificationStatus::verified",
    "NativeTimerCreateReadbackVerificationStatus::alreadyVerified",
    "NativeTimerCreateReadbackVerificationStatus::correlationNotFound",
    "NativeTimerCreateReadbackVerificationStatus::correlationAmbiguous",
    "NativeTimerCreateReadbackVerificationStatus::reconciliationRequired",
    "NativeTimerCreateReadbackVerificationStatus::nativeIdentityConflict",
    "NativeTimerCreateReadbackVerificationStatus::assignmentBindingConflict",
    "test_native_timer_create_readback_verification_service passed",
]:
    if token not in test:
        raise SystemExit(f"missing CREATE verification regression marker: {token}")

for token in [
    "authoritative complete inventory",
    "exactly one managed correlation",
    "no blind retry",
    "NativeTimerBinding",
    "outcome_unknown",
]:
    if token not in doc:
        raise SystemExit(f"missing CREATE verification documentation marker: {token}")

for token in [
    "test-phase64-native-timer-create-readback-verification-architecture",
    "test-phase64-native-timer-create-readback-verification:",
    "test-fast: test-phase64-native-timer-create-readback-verification",
    "test-architecture: test-phase64-native-timer-create-readback-verification-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing CREATE verification make marker: {token}")

include_line = "include mk/phase64-native-timer-create-readback-verification-tests.mk"
if include_line not in makefile:
    raise SystemExit("CREATE verification make fragment is not included")

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
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk"
        }:
            continue
        if "NativeTimerCreateReadbackVerificationService" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature CREATE readback runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer CREATE readback verification check passed")
