#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "core/timers/include/NativeTimerAbsenceReadbackExpectation.h"
SOURCE = ROOT / "core/timers/src/NativeTimerAbsenceReadbackExpectation.cpp"
TEST = ROOT / "core/timers/tests/test_native_timer_absence_readback_expectation.cpp"
DOC = ROOT / "docs/development/phase-64-native-timer-absence-readback-expectation.md"
MK = ROOT / "mk/phase64-native-timer-absence-readback-expectation-tests.mk"
MAKEFILE = ROOT / "Makefile"

for path in [HEADER, SOURCE, TEST, DOC, MK, MAKEFILE]:
    if not path.is_file():
        raise SystemExit(f"missing Slice-18 file: {path.relative_to(ROOT)}")

header = HEADER.read_text(encoding="utf-8")
source = SOURCE.read_text(encoding="utf-8")
test = TEST.read_text(encoding="utf-8")
doc = DOC.read_text(encoding="utf-8")
mk = MK.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

for token in [
    "NativeTimerAbsenceReadbackExpectation",
    "NativeTimerReadbackOperationState",
    "operationId",
    "nativeTimerBindingId",
    "expectedBindingRevision",
    "backendId",
    "backendGeneration",
    "backendNativeTimerId",
    "readbackNotBefore",
    "nativeTimerAbsenceReadbackExpectationValid",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-18 header marker: {token}")

for token in [
    "kMaxIdentityLength = 160",
    "NativeTimerReadbackOperationState::executedUnverified",
    "NativeTimerReadbackOperationState::outcomeUnknown",
    "expectation.backendGeneration != 0",
    "expectation.readbackNotBefore > 0",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-18 source marker: {token}")

for forbidden in [
    "NativeTimerObservedState expectedState",
    "expectedFingerprint",
    "NativeTimerInventoryEvidence",
    "NativeTimerBindingRepository",
    "sqlite3_",
    "VdrTimer",
    "RestfulApi",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in header + source + test:
        raise SystemExit(f"premature Slice-18 dependency: {forbidden}")

for token in [
    "NativeTimerReadbackOperationState::executedUnverified",
    "NativeTimerReadbackOperationState::outcomeUnknown",
    "static_cast<NativeTimerReadbackOperationState>(99)",
    "std::string(160, 'o')",
    "std::string(160, 'n')",
    "test_native_timer_absence_readback_expectation passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-18 regression marker: {token}")

for token in [
    "Native Timer Expected Absence Readback Contract",
    "not absence evidence",
    "`readbackNotBefore`",
    "No new operation lifecycle vocabulary",
    "no `NativeTimerObservedState` or fingerprint",
    "absence verifier",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-18 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-absence-readback-expectation-architecture",
    "test-phase64-native-timer-absence-readback-expectation:",
    "core/timers/src/NativeTimerAbsenceReadbackExpectation.cpp",
    "core/timers/tests/test_native_timer_absence_readback_expectation.cpp",
    "test-fast: test-phase64-native-timer-absence-readback-expectation",
    "test-architecture: test-phase64-native-timer-absence-readback-expectation-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-18 make marker: {token}")

if "include mk/phase64-native-timer-absence-readback-expectation-tests.mk" not in makefile:
    raise SystemExit("Slice-18 make fragment is not included")

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
        if "NativeTimerAbsenceReadbackExpectation" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature Slice-18 runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer absence readback expectation check passed")
print(
    "Slice-18 boundary: expected absence operation contract only; "
    "verification, lifecycle orchestration and native mutation deferred"
)
