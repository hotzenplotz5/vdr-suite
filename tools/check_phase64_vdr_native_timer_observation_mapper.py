#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

header_path = "core/vdr/include/VdrNativeTimerObservationMapper.h"
source_path = "core/vdr/src/VdrNativeTimerObservationMapper.cpp"
test_path = "core/vdr/tests/test_vdr_native_timer_observation_mapper.cpp"
doc_path = "docs/development/phase-64-vdr-native-timer-observation-mapper.md"
make_path = "mk/phase64-timer-intent-tests.mk"
required = [
    "core/vdr/include/VdrTimer.h",
    "core/timers/include/NativeTimerBinding.h",
    "core/timers/src/NativeTimerBinding.cpp",
    header_path,
    source_path,
    test_path,
    doc_path,
    make_path,
]
for relative in required:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Slice-11 native Timer observation file: {relative}")

header = (ROOT / header_path).read_text(encoding="utf-8")
source = (ROOT / source_path).read_text(encoding="utf-8")
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
make = (ROOT / make_path).read_text(encoding="utf-8")

for token in [
    "VdrNativeTimerObservation",
    "VdrNativeTimerObservationMapStatus",
    "VdrNativeTimerObservationMapResult",
    "class VdrNativeTimerObservationMapper",
    "backendId",
    "backendGeneration",
    "backendNativeTimerId",
    "observedAt",
    "NativeTimerObservedState",
    "observedFingerprint",
]:
    if token not in header:
        raise SystemExit(f"missing Slice-11 mapper header marker: {token}")

for token in [
    "backendGeneration == 0",
    "timer.id",
    "state.channelId = timer.channelId",
    "state.eventId = timer.eventId",
    "state.title = timer.title",
    "state.directory = timer.directory",
    "state.day = timer.day",
    "state.weekdays = timer.weekdays",
    "state.startTime = timer.startTime",
    "state.endTime = timer.endTime",
    "state.enabled = timer.enabled",
    "nativeTimerObservedStateValid(state)",
    "nativeTimerObservedStateFingerprint(state)",
]:
    if token not in source:
        raise SystemExit(f"missing Slice-11 mapper source marker: {token}")

for forbidden in [
    "timer.channelName",
    "timer.subtitle",
    "timer.aux",
    "NativeTimerBindingRepository",
    "sqlite3_",
    "TimerAssignmentPlanner",
    "TimerAssignmentSchedulingService",
    "NativeTimerBindingOwnership",
    "NativeTimerBindingDriftState",
    "BackendAgentCommand",
    "SuiteBridge",
    "SVDRP",
    "mutations=enabled",
]:
    if forbidden in source:
        raise SystemExit(f"premature Slice-11 mapper boundary crossing: {forbidden}")

for token in [
    'mapped.observation.backendGeneration == 7',
    'mapped.observation.backendNativeTimerId == "timer:42"',
    'mapped.observation.observedState.startTime == "930"',
    'paddedMapped.observation.observedState.startTime == "0930"',
    "paddedMapped.observation.observedFingerprint ==",
    "providerPrivateChanged.channelName",
    "providerPrivateChanged.subtitle",
    "providerPrivateChanged.aux",
    "materialChanged.enabled = false",
    "invalidObservedState",
    "invalidNativeTimerIdentity",
    "invalidBackendIdentity",
    "invalidBackendGeneration",
    "invalidObservedAt",
]:
    if token not in test:
        raise SystemExit(f"missing Slice-11 mapper regression marker: {token}")

for token in [
    "VDR Native Timer Observation Mapper",
    "mapping-only",
    "explicit backend/generation/time evidence",
    "VdrSnapshot",
    "no backend generation",
    "Provider-private fields stay below the boundary",
    "No repository write",
    "does not infer or set",
    "`core/vdr` header allowed",
    "no installed runtime behavior",
    "readback application service",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-11 documentation marker: {token}")

for token in [
    "test-phase64-vdr-native-timer-observation-mapper-architecture",
    "test-phase64-vdr-native-timer-observation-mapper:",
    "core/vdr/src/VdrNativeTimerObservationMapper.cpp",
    "core/vdr/tests/test_vdr_native_timer_observation_mapper.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make:
        raise SystemExit(f"missing Slice-11 test-graph marker: {token}")

print("Phase-64 VDR native Timer observation mapper check passed")
print(
    "Slice-11 boundary: explicit generation-fenced VDR read mapping only; "
    "repository application/reconciliation/native mutation deferred")
