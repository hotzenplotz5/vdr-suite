#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

header_path = "core/timers/include/NativeTimerReadbackExpectation.h"
source_path = "core/timers/src/NativeTimerReadbackExpectation.cpp"
test_path = "core/timers/tests/test_native_timer_readback_expectation.cpp"
doc_path = "docs/development/phase-64-native-timer-readback-expectation.md"
mk_path = "mk/phase64-native-timer-readback-expectation-tests.mk"
makefile_path = "Makefile"

for relative in [
    "core/timers/include/NativeTimerBinding.h",
    header_path,
    source_path,
    test_path,
    doc_path,
    mk_path,
    makefile_path,
]:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Slice-13 expected-readback file: {relative}")

header = (ROOT / header_path).read_text(encoding="utf-8")
source = (ROOT / source_path).read_text(encoding="utf-8")
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
mk = (ROOT / mk_path).read_text(encoding="utf-8")
makefile = (ROOT / makefile_path).read_text(encoding="utf-8")

for token in [
    "NativeTimerReadbackOperationState",
    "executedUnverified",
    "outcomeUnknown",
    "NativeTimerReadbackExpectation",
    "operationId",
    "nativeTimerBindingId",
    "expectedBindingRevision",
    "backendId",
    "backendGeneration",
    "backendNativeTimerId",
    "readbackNotBefore",
    "expectedState",
    "expectedFingerprint",
    "nativeTimerReadbackExpectationValid",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-13 header marker: {token}")

for token in [
    '"executed_unverified"',
    '"outcome_unknown"',
    "operationStateValid(expectation.operationState)",
    "expectation.backendGeneration == 0",
    "expectation.readbackNotBefore <= 0",
    "nativeTimerObservedStateValid(expectation.expectedState)",
    "expectation.expectedFingerprint",
    "nativeTimerObservedStateFingerprint(expectation.expectedState)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-13 source marker: {token}")

for forbidden in [
    "NativeTimerBindingRepository",
    "NativeTimerBindingReadbackService",
    "VdrTimer",
    "sqlite3_",
    "TimerAssignment",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "externalFieldChange",
    "externalDelete",
    "mutations=enabled",
]:
    if forbidden in header + source:
        raise SystemExit(f"premature Slice-13 boundary crossing: {forbidden}")

for token in [
    "nativeTimerReadbackExpectationValid(expectation)",
    "NativeTimerReadbackOperationState::outcomeUnknown",
    "expectation.backendGeneration = 0",
    "expectation.readbackNotBefore = 0",
    'expectation.expectedFingerprint += "-different"',
    'normalizedB.expectedState.startTime = "0930"',
    "normalizedA.expectedFingerprint == normalizedB.expectedFingerprint",
    "std::string(161, 'x')",
    "test_native_timer_readback_expectation passed",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-13 regression marker: {token}")

for token in [
    "Native Timer Expected Readback Contract",
    "`readbackNotBefore`",
    "stale pre-operation snapshot",
    "`executed_unverified`",
    "`outcome_unknown`",
    "Present readback only",
    "No drift classification",
    "snapshot-completeness",
    "operation-aware present-readback verifier",
    "no `mutations=enabled`",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-13 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-readback-expectation-architecture",
    "test-phase64-native-timer-readback-expectation:",
    "core/timers/src/NativeTimerReadbackExpectation.cpp",
    "core/timers/tests/test_native_timer_readback_expectation.cpp",
    "test-fast: test-phase64-native-timer-readback-expectation",
    "test-architecture: test-phase64-native-timer-readback-expectation-architecture",
]:
    if token not in mk:
        raise SystemExit(f"missing Slice-13 make marker: {token}")

include_line = "include mk/phase64-native-timer-readback-expectation-tests.mk"
if include_line not in makefile:
    raise SystemExit("Slice-13 make fragment is not included")

for scan_root in [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core" / "agent",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "runtime",
    ROOT / "core" / "vdr",
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
        if "NativeTimerReadbackExpectation" in path.read_text(
            encoding="utf-8", errors="ignore"
        ):
            raise SystemExit(
                "premature expected-readback runtime wiring: "
                + str(path.relative_to(ROOT))
            )

print("Phase-64 native Timer expected-readback check passed")
print(
    "Slice-13 boundary: operation expectation only; "
    "verification, absence and native mutation deferred"
)
