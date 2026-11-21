#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

header_path = "core/timers/include/NativeTimerBindingReadbackService.h"
source_path = "core/timers/src/NativeTimerBindingReadbackService.cpp"
test_path = "core/timers/tests/test_native_timer_binding_readback_service.cpp"
doc_path = "docs/development/phase-64-native-timer-binding-readback-service.md"
make_path = "mk/phase64-timer-intent-tests.mk"
required = [
    "core/timers/include/NativeTimerObservation.h",
    "core/timers/include/NativeTimerBindingRepository.h",
    header_path, source_path, test_path, doc_path, make_path,
]
for relative in required:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Slice-12 readback file: {relative}")

header = (ROOT / header_path).read_text(encoding="utf-8")
source = (ROOT / source_path).read_text(encoding="utf-8")
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
make = (ROOT / make_path).read_text(encoding="utf-8")

for token in [
    "NativeTimerBindingReadbackStatus",
    "refreshed",
    "alreadyCurrent",
    "unboundObservation",
    "staleGeneration",
    "staleObservation",
    "reconciliationRequired",
    "repositoryConflict",
    "class NativeTimerBindingReadbackService",
    "applyPresentObservation",
    "NativeTimerObservation",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-12 readback header marker: {token}")

for token in [
    "nativeTimerObservationValid(observation)",
    "repository_.findByBackendNativeTimer(",
    "observation.backendGeneration < current.backendGeneration",
    "observation.observedAt < current.lastObservedAt",
    "current.missingSince != 0",
    "observation.observedFingerprint != current.observedFingerprint",
    "nativeTimerObservedStateSameExceptEventId(",
    "eventIdOnlyDrift",
    "NativeTimerBindingReadbackStatus::reconciliationRequired",
    "next.backendGeneration = observation.backendGeneration",
    "next.lastObservedAt = observation.observedAt",
    "repository_.update(",
    "next, current.bindingRevision, passiveFreshnessRefresh",
    "NativeTimerBindingRepositoryStatus::conflict",
    "NativeTimerBindingReadbackStatus::repositoryConflict",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-12 readback source marker: {token}")

for forbidden in [
    "next.observedState =",
    "next.ownership =",
    "next.timerAssignmentId =",
    "next.missingSince =",
    "next.driftState =",
    "VdrTimer",
    "sqlite3_",
    "TimerAssignmentPlanner",
    "TimerAssignmentSchedulingService",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in source + header:
        raise SystemExit(f"premature Slice-12 boundary crossing: {forbidden}")

# Phase-69 successor: a VDR EPG event association is transient, not a
# native timer configuration mutation. Exactly one narrowly fenced metadata
# refresh may update eventId and its canonical fingerprint. Preserve the
# historic ban against any other passive observed-state rewrite.
epg_guard = """    if (eventIdOnlyDrift)
    {
        next.observedState.eventId = observation.observedState.eventId;
        next.observedFingerprint = nativeTimerObservedStateFingerprint(
            next.observedState);
    }"""
if source.count("next.observedFingerprint =") != 1 or (
        source.count("next.observedState.eventId =") != 1 or
        epg_guard not in source or
        "passiveFreshnessRefresh &&" not in source or
        "nativeTimerObservedStateSameExceptEventId(" not in source):
    raise SystemExit("unfenced EPG-only native timer readback mutation")

for token in [
    "NativeTimerBindingReadbackStatus::refreshed",
    'refreshed.binding.bindingRevision == "2"',
    'refreshed.binding.observedState.startTime == "930"',
    "NativeTimerBindingReadbackStatus::alreadyCurrent",
    "NativeTimerBindingReadbackStatus::staleGeneration",
    "NativeTimerBindingReadbackStatus::staleObservation",
    "NativeTimerBindingReadbackStatus::reconciliationRequired",
    "afterChanged.binding.bindingRevision == \"2\"",
    "missingAfter.binding.missingSince == 1050",
    "driftRefresh.binding.driftState == NativeTimerBindingDriftState::externalFieldChange",
    "NativeTimerBindingReadbackStatus::unboundObservation",
    "NativeTimerBindingReadbackStatus::invalid",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-12 readback regression marker: {token}")

for token in [
    "NativeTimerBinding Present Readback Application",
    "**present** observations",
    "Changed state is reconciliation evidence",
    "durable absence evidence",
    "Safe unchanged refresh",
    "repositoryConflict",
    "Existing drift is preserved",
    "no installed runtime path",
    "changed-state reconciliation classification",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-12 documentation marker: {token}")

for token in [
    "test-phase64-native-timer-binding-readback-architecture",
    "test-phase64-native-timer-binding-readback:",
    "core/timers/src/NativeTimerBindingReadbackService.cpp",
    "core/timers/tests/test_native_timer_binding_readback_service.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make:
        raise SystemExit(f"missing Slice-12 test-graph marker: {token}")

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
        if "NativeTimerBindingReadbackService" in path.read_text(
            encoding="utf-8", errors="ignore"):
            # Phase-69 successor: only the productive daemon polling owner
            # may consume the service, and only after strict live evidence.
            if (path.relative_to(ROOT) ==
                    Path("core/daemon/src/DaemonRuntimePolling.cpp")):
                poll = path.read_text(encoding="utf-8")
                if all(marker in poll for marker in [
                    "RestfulApiNativeTimerModifyReadbackReader",
                    "listManagedForFreshness(",
                    "agent->backendGeneration < binding.backendGeneration",
                    "request.backendGeneration = agent->backendGeneration",
                    "stillAgent->backendGeneration ==\n"
                    "                                    request.backendGeneration",
                    "observed.observation, true",
                ]):
                    continue
            raise SystemExit(
                "premature NativeTimerBinding readback runtime wiring: "
                + str(path.relative_to(ROOT)))

print("Phase-64 NativeTimerBinding present readback check passed")
print(
    "Slice-12 successor: unchanged/config-preserving EPG-only passive refresh; "
    "changed/missing timer configuration and native mutation still deferred")
