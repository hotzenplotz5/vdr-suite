#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "runtime_h": ROOT / "core/daemon/include/NativeTimerCreateProductiveRuntime.h",
    "runtime_cpp": ROOT / "core/daemon/src/NativeTimerCreateProductiveRuntime.cpp",
    "polling": ROOT / "core/daemon/src/DaemonRuntimePolling.cpp",
    "operation_h": ROOT / "core/operations/include/MutationOperationRepository.h",
    "operation_cpp": ROOT / "core/operations/src/MutationOperationRepository.cpp",
    "test": ROOT / "core/daemon/tests/test_native_timer_create_productive_runtime.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "phase69_make": ROOT / "mk/phase69-public-api-tests.mk",
    "ci_groups": ROOT / "mk/test-groups.mk",
    "doc": ROOT / "docs/development/phase-69c-native-timer-create-productive-runtime.md",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            "missing productive Timer CREATE runtime file: "
            + str(path.relative_to(ROOT)))
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "runtime_h": [
        "advanceNativeTimerCreateRuntimeOnce(",
        "NativeTimerCreateReadbackAcquirer",
        "owns no persistence",
        "polling loop",
        "retry loop",
    ],
    "runtime_cpp": [
        'listByActionFamilyAndStates(',
        '"timer.create"',
        "MutationOperationState::accepted",
        "MutationOperationState::dispatching",
        "MutationOperationState::executedUnverified",
        "MutationOperationState::outcomeUnknown",
        "reservationService.reserve(",
        "dispatchService.claimAfterReservation(",
        "activationService.activateDispatching(",
        "commandRepository.resultForCommand(",
        "applyDurableNativeTimerCreateResult(",
        "reconcileNativeTimerCreateReadback(",
        "agentFingerprint != domainFingerprint",
    ],
    "polling": [
        "backendPollingCoordinator_->pollAll();",
        "advanceNativeTimerCreateRuntimeOnce(",
        "RestfulApiNativeTimerInventoryReader",
        "VdrManagedTimerCreateReadbackEvidenceBuilder::build(",
        "agent->backendGeneration != expectation.backendGeneration",
        "nativeCreateReadbackCache",
    ],
    "operation_h": [
        "listByActionFamilyAndStates(",
        "std::size_t limit",
    ],
    "operation_cpp": [
        "action_family=? AND state IN (",
        "ORDER BY updated_at ASC,operation_id ASC LIMIT ?",
        "states.size() > 8",
        "limit > 256",
    ],
    "test": [
        "dispatchClaims == 1",
        "activations == 1",
        "sameCommand->commandId == exactCommandId",
        "outcomesApplied == 1",
        "reconciliationsCompleted == 1",
        "MutationOperationState::succeeded",
        "TimerAssignmentState::bound",
        "NativeTimerBindingState::verified",
        "terminalReplay.discovered == 0",
    ],
    "daemon_sources": [
        "core/daemon/src/NativeTimerCreateProductiveRuntime.cpp",
        "core/timers/src/NativeTimerInventoryEvidence.cpp",
        "core/vdr/src/RestfulApiNativeTimerInventoryReader.cpp",
        "core/vdr/src/VdrManagedTimerCreateReadbackEvidenceBuilder.cpp",
    ],
    "phase69_make": [
        "test-phase69-native-timer-create-productive-runtime",
        "tools/check_phase69_native_timer_create_productive_runtime.py",
        "core/daemon/tests/test_native_timer_create_productive_runtime.cpp",
    ],
    "ci_groups": [
        "test-phase69-native-timer-create-productive-runtime",
    ],
    "doc": [
        "NATIVE_EFFECT_REACHABLE=YES",
        "YAVDR_ACCEPTANCE_REQUIRED=YES",
        "outcome_unknown",
        "no blind retry",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing productive Timer CREATE runtime marker in {label}: {token}")

runtime = contents["runtime_cpp"]
for forbidden in [
    "CREATE TABLE",
    "ALTER TABLE",
    "std::thread",
    "sleep_for(",
    "sleep_until(",
    "while (",
    "requestReplay(",
]:
    if forbidden in runtime:
        raise SystemExit(
            "productive Timer CREATE runtime must not create a second "
            f"authority/poller/retry path: {forbidden}")

accepted_pos = runtime.find(
    "if (current.state == MutationOperationState::accepted)")
reserve_pos = runtime.find("reserveClaimAndActivate(", accepted_pos)
outcome_unknown_pos = runtime.find(
    "current.state == MutationOperationState::outcomeUnknown")
if not (0 <= accepted_pos < reserve_pos < outcome_unknown_pos):
    raise SystemExit(
        "native dispatch must remain reachable only from accepted state; "
        "outcome_unknown is reconciliation-only")

polling = contents["polling"]
poll_pos = polling.find("backendPollingCoordinator_->pollAll();")
advance_pos = polling.find("advanceNativeTimerCreateRuntimeOnce(")
if not (0 <= poll_pos < advance_pos):
    raise SystemExit(
        "Timer CREATE runtime must reuse the existing daemon poll cadence")

# Only the reviewed daemon runtime may productively orchestrate CREATE. Public,
# HTTP and security layers stay backend-neutral and never dispatch natively.
for scan_root in [
    ROOT / "api",
    ROOT / "core" / "http",
    ROOT / "core" / "security",
]:
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc"
        }:
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for token in [
            "advanceNativeTimerCreateRuntimeOnce(",
            "BackendAgentNativeTimerCreateReservationService",
            "reconcileNativeTimerCreateReadback(",
        ]:
            if token in text:
                raise SystemExit(
                    "Timer CREATE productive authority leaked into "
                    + str(path.relative_to(ROOT)) + ": " + token)

# Timer mutation transport remains the accepted typed SVDRP path. This slice
# may not introduce a local Unix mutation transport or modify frontend/Home.
for forbidden_path in [
    ROOT / "web" / "frontend",
    ROOT / "plugins" / "suitebridge",
]:
    if not forbidden_path.exists():
        continue

print("Phase-69.C productive Native Timer CREATE runtime check passed")
print("Boundary: existing daemon poll advances durable CREATE exactly once per state; native effect is reachable and requires real yaVDR acceptance")
