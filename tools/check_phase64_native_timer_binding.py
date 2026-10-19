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
    "normalizedNativeHhmm",
    "bounded(state.day, kMaxIdentityLength)",
    "appendField(fingerprint, normalizedNativeHhmm(state.startTime))",
    "appendField(fingerprint, normalizedNativeHhmm(state.endTime))",
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
    "paddedMidnight",
    'paddedMidnight.startTime = "0000"',
    'paddedMidnight.endTime = "0930"',
    "== nativeTimerObservedStateFingerprint(mapperCompatible)",
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
    "four-digit HHMM",
    "fingerprint-equivalent",
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
# Slice 9 originally prohibited every productive runtime reference. Phase 69.C
# may now compose the already accepted repository/fulfillment owners in exactly
# the Control-Plane daemon files below, but only when the dedicated successor
# guard proves the public mutation and fulfillment calls remain dormant.
allowed_phase69_daemon_paths = {
    Path("core/daemon/include/DaemonRuntime.h"),
    Path("core/daemon/src/DaemonRuntimeInitialization.cpp"),
}
allowed_phase69_readback_paths = {
    Path("core/daemon/include/NativeTimerCreateReadbackReconciliation.h"),
    Path("core/daemon/src/NativeTimerCreateReadbackReconciliation.cpp"),
    Path("core/daemon/tests/test_native_timer_create_readback_reconciliation.cpp"),
}
successor_guard = ROOT / "tools/check_phase69_native_timer_create_fulfillment_runtime.py"
successor_valid = False
if successor_guard.is_file():
    successor = successor_guard.read_text(encoding="utf-8")
    successor_valid = all(marker in successor for marker in [
        "NativeTimerBindingRepository",
        "TimerAssignmentFulfillmentService",
        "CREATE fulfillment runtime must remain dormant in this slice",
        "timerAssignmentFulfillmentService_->beginProvisioning(",
    ])

readback_successor_guard = (
    ROOT / "tools/check_phase69_native_timer_create_readback_reconciliation.py"
)
readback_successor_valid = False
if readback_successor_guard.is_file():
    readback_successor = readback_successor_guard.read_text(encoding="utf-8")
    readback_successor_valid = all(marker in readback_successor for marker in [
        "reconcileNativeTimerCreateReadback(",
        "verificationService.verify(expectation, readbackEvidence)",
        "must remain productively",
    ])

productive_successor_guard = (
    ROOT / "tools/check_phase69_native_timer_create_productive_runtime.py"
)
productive_successor_valid = False
if productive_successor_guard.is_file():
    productive_successor = productive_successor_guard.read_text(encoding="utf-8")
    productive_successor_valid = all(marker in productive_successor for marker in [
        "core/daemon/tests/test_native_timer_create_productive_runtime.cpp",
        "NativeTimerBindingOwnership::managed",
        "reconciliationsCompleted == 1",
    ])
# The native DELETE successor is a distinct, provider- and revision-fenced
# Control-Plane path. Do not loosen the old blanket prohibition: admit only
# explicit audited DELETE source paths when the specific safety markers exist.
delete_runtime = ROOT / "core/daemon/src/NativeTimerDeleteProductiveRuntime.cpp"
delete_application = ROOT / "core/daemon/src/NativeTimerDeleteResultOutcomeApplication.cpp"
delete_runtime_valid = (
    delete_runtime.is_file() and delete_application.is_file()
    and all(marker in delete_runtime.read_text(encoding="utf-8") for marker in [
        "publicNativeTimerDeletePreflight(", "applyDurableNativeTimerDeleteResult(",
        "verification.verify(", "completion.complete(",
        "MutationOperationState::outcomeUnknown",
        "NativeTimerDeleteAssignmentService",
    ])
    and all(marker in delete_application.read_text(encoding="utf-8") for marker in [
        "backendAgentNativeTimerDeleteParseResultEvidence(",
        "NativeTimerDeleteDispatchService", "dispatchService.applyOutcome(",
        "NativeTimerBindingOwnership::managed",
        "NativeTimerBindingOwnership::adopted",
    ])
)
delete_runtime_paths = {
    Path("core/daemon/include/NativeTimerDeleteProductiveRuntime.h"),
    Path("core/daemon/src/NativeTimerDeleteProductiveRuntime.cpp"),
    Path("core/daemon/include/NativeTimerDeleteResultOutcomeApplication.h"),
    Path("core/daemon/src/NativeTimerDeleteResultOutcomeApplication.cpp"),
}

modify_runtime_paths = {
    Path("core/daemon/include/NativeTimerModifyProductiveRuntime.h"),
    Path("core/daemon/src/NativeTimerModifyProductiveRuntime.cpp"),
    Path("core/daemon/include/NativeTimerModifyResultOutcomeApplication.h"),
    Path("core/daemon/src/NativeTimerModifyResultOutcomeApplication.cpp"),
}
modify_source = ROOT / "core/daemon/src/NativeTimerModifyProductiveRuntime.cpp"
modify_outcome = ROOT / "core/daemon/src/NativeTimerModifyResultOutcomeApplication.cpp"
modify_runtime_valid = (
    modify_source.is_file() and modify_outcome.is_file()
    and all(token in modify_source.read_text(encoding="utf-8") for token in [
        "b.bindingRevision != payload.expectedBindingRevision",
        "b.observedFingerprint != payload.expectedCurrentFingerprint",
        "agents.findAgentForBackend(op.backendId)",
        "assigner.assign(",
        "applyDurableNativeTimerModifyResult(",
        "readback(applied.outcome.expectation)",
        "verifier.verify(",
        "completion.complete(",
    ])
    and all(token in modify_outcome.read_text(encoding="utf-8") for token in [
        "backendAgentNativeTimerModifyParseResultEvidence(",
        "NativeTimerBindingOwnership::managed",
        "NativeTimerBindingOwnership::adopted",
        "dispatcher.applyOutcome(",
    ])
)

productive_paths = {
    Path("core/daemon/include/NativeTimerCreateProductiveRuntime.h"),
    Path("core/daemon/src/NativeTimerCreateProductiveRuntime.cpp"),
    Path("core/daemon/tests/test_native_timer_create_productive_runtime.cpp"),
}

for scan_root in forbidden_roots:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in text_suffixes:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if "NativeTimerBinding" not in text:
            continue
        relative = path.relative_to(ROOT)
        if relative in allowed_phase69_daemon_paths:
            if not successor_valid:
                raise SystemExit(
                    "Phase-69.C NativeTimerBinding daemon wiring requires "
                    "the exact dormant fulfillment successor guard")
            continue
        if relative in allowed_phase69_readback_paths:
            if not readback_successor_valid:
                raise SystemExit(
                    "Phase-69.C NativeTimerBinding readback wiring requires "
                    "the exact dormant readback-reconciliation successor guard")
            continue
        if relative in delete_runtime_paths:
            if not delete_runtime_valid:
                raise SystemExit(
                    "NativeTimerBinding DELETE runtime needs its explicit "
                    "typed-evidence and readback successor guard")
            continue
        if relative in modify_runtime_paths:
            if not modify_runtime_valid:
                raise SystemExit("native MODIFY binding successor lacks durable evidence or readback")
            continue
        if relative in productive_paths:
            if not productive_successor_valid:
                raise SystemExit(
                    "Phase-69.C NativeTimerBinding productive wiring requires "
                    "the exact native-effect successor guard")
            continue
        raise SystemExit(
            "premature NativeTimerBinding runtime wiring: "
            + str(relative))

print("Phase-64 NativeTimerBinding contract check passed")
print(
    "Slice-9 boundary: domain value/fingerprint contract only; "
    "persistence/reconciliation/native mutation deferred")
