#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

required_files = [
    "core/timers/include/NativeTimerBinding.h",
    "core/timers/src/NativeTimerBinding.cpp",
    "core/timers/tests/test_native_timer_binding.cpp",
    "docs/development/phase-64-native-timer-binding-contract.md",
    "mk/phase64-timer-intent-tests.mk",
]

for relative in required_files:
    if not (ROOT / relative).is_file():
        raise SystemExit(
            f"missing Phase-64 NativeTimerBinding contract file: {relative}")

header = (ROOT / required_files[0]).read_text(encoding="utf-8")
source = (ROOT / required_files[1]).read_text(encoding="utf-8")
test = (ROOT / required_files[2]).read_text(encoding="utf-8")
doc = (ROOT / required_files[3]).read_text(encoding="utf-8")
make_fragment = (ROOT / required_files[4]).read_text(encoding="utf-8")

for token in [
    "NativeTimerBindingOwnership",
    "NativeTimerBindingDriftState",
    "NativeTimerObservedState",
    "NativeTimerBinding",
    "nativeTimerObservedStateValid",
    "nativeTimerObservedStateFingerprint",
    "nativeTimerBindingValid",
    "nativeTimerBindingRevisionMatches",
    "backendNativeTimerId",
    "timerAssignmentId",
    "lastVerifiedOperationId",
    "missingSince",
]:
    if token not in header:
        raise SystemExit(f"missing NativeTimerBinding header marker: {token}")

for token in [
    '"managed"',
    '"adopted"',
    '"external"',
    '"orphaned_managed"',
    '"ambiguous"',
    '"expected_transition"',
    '"external_field_change"',
    '"external_disable"',
    '"external_delete"',
    '"native_identity_changed"',
    '"native-timer-observed-state/1|"',
    "validNativeHhmm",
    "bounded(state.day, kMaxIdentityLength)",
    "binding.backendGeneration == 0",
    "binding.observedFingerprint",
    "nativeTimerObservedStateFingerprint(binding.observedState)",
    "binding.missingSince > binding.lastObservedAt",
]:
    if token not in source:
        raise SystemExit(f"missing NativeTimerBinding source marker: {token}")

for token in [
    "NativeTimerBindingOwnership::managed",
    "NativeTimerBindingOwnership::adopted",
    "NativeTimerBindingOwnership::external",
    "NativeTimerBindingOwnership::orphanedManaged",
    "NativeTimerBindingOwnership::ambiguous",
    'mapperCompatible.startTime = "0"',
    'mapperCompatible.endTime = "930"',
    "nativeTimerObservedStateFingerprint(disabled) != fingerprint",
    "externalWithAssignment",
    "managedWithoutAssignment",
    "fingerprintMismatch",
    "missingWithoutDrift",
    "deleteWithoutMissing",
    "expectedMissing",
    "enabledDisableDrift",
    "zeroGeneration",
    "noNativeId",
    "futureMissing",
    "nativeTimerBindingRevisionMatches",
]:
    if token not in test:
        raise SystemExit(f"missing NativeTimerBinding regression marker: {token}")

for token in [
    "NativeTimerBinding Domain Contract",
    "contract only",
    "backendNativeTimerId",
    "backendGeneration",
    "No `VdrTimer` dependency",
    "No plugin-specific `aux` field",
    "native-timer-observed-state/1",
    "current RESTfulAPI Timer mapper",
    "day` as optional",
    "one-to-four digit",
    "missingSince",
    "external_delete",
    "lastVerifiedOperationId",
    "no installed runtime path",
    "NativeTimerBinding persistence",
]:
    if token not in doc:
        raise SystemExit(f"missing Phase-64 Slice-9 statement: {token}")

for token in [
    "test-phase64-native-timer-binding-contract-architecture",
    "test-phase64-native-timer-binding-contract:",
    "core/timers/src/NativeTimerBinding.cpp",
    "core/timers/tests/test_native_timer_binding.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make_fragment:
        raise SystemExit(f"missing NativeTimerBinding test-graph marker: {token}")

contract_text = header + source + test
for forbidden in [
    "#include <sqlite3.h>",
    "sqlite3_",
    "Repository",
    "TimerAssignmentPlanner",
    "TimerAssignmentSchedulingService",
    "VdrTimer",
    "BackendAgentCommand",
    "SuiteBridge",
    "RESTfulAPI",
    "SVDRP",
    "mutations=enabled",
    "std::string aux",
    ".aux",
]:
    if forbidden in contract_text:
        raise SystemExit(f"premature NativeTimerBinding boundary crossing: {forbidden}")

forbidden_roots = [
    ROOT / "apps",
    ROOT / "api",
    ROOT / "core" / "agent",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "runtime",
    ROOT / "core" / "vdr",
    ROOT / "vdr-plugin-suite-bridge",
]
text_suffixes = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc", ".mk",
    ".conf", ".service",
}
for scan_root in forbidden_roots:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "NativeTimerBinding" in text:
            raise SystemExit(
                "premature NativeTimerBinding runtime wiring: "
                + str(path.relative_to(ROOT)))

print("Phase-64 NativeTimerBinding contract check passed")
print(
    "Slice-9 boundary: domain value/fingerprint contract only; "
    "persistence/reconciliation/native mutation deferred")
