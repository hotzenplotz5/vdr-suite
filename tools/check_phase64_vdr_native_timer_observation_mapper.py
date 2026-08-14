#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

observation_header = "core/timers/include/NativeTimerObservation.h"
observation_source = "core/timers/src/NativeTimerObservation.cpp"
mapper_header = "core/vdr/include/VdrNativeTimerObservationMapper.h"
mapper_source = "core/vdr/src/VdrNativeTimerObservationMapper.cpp"
test_path = "core/vdr/tests/test_vdr_native_timer_observation_mapper.cpp"
doc_path = "docs/development/phase-64-vdr-native-timer-observation-mapper.md"
make_path = "mk/phase64-timer-intent-tests.mk"
required = [
    "core/vdr/include/VdrTimer.h",
    "core/timers/include/NativeTimerBinding.h",
    "core/timers/src/NativeTimerBinding.cpp",
    observation_header,
    observation_source,
    mapper_header,
    mapper_source,
    test_path,
    doc_path,
    make_path,
]
for relative in required:
    if not (ROOT / relative).is_file():
        raise SystemExit(f"missing Slice-11 native Timer observation file: {relative}")

observation = (ROOT / observation_header).read_text(encoding="utf-8")
observation_impl = (ROOT / observation_source).read_text(encoding="utf-8")
header = (ROOT / mapper_header).read_text(encoding="utf-8")
source = (ROOT / mapper_source).read_text(encoding="utf-8")
test = (ROOT / test_path).read_text(encoding="utf-8")
doc = (ROOT / doc_path).read_text(encoding="utf-8")
make = (ROOT / make_path).read_text(encoding="utf-8")

for token in [
    "struct NativeTimerObservation",
    "backendId",
    "backendGeneration",
    "backendNativeTimerId",
    "observedAt",
    "NativeTimerObservedState",
    "observedFingerprint",
    "nativeTimerObservationValid",
]:
    if token not in observation:
        raise SystemExit(f"missing backend-neutral observation marker: {token}")

for token in [
    "observation.backendGeneration == 0",
    "observation.observedAt <= 0",
    "nativeTimerObservedStateValid(observation.observedState)",
    "nativeTimerObservedStateFingerprint(observation.observedState)",
    "observation.observedFingerprint == fingerprint",
]:
    if token not in observation_impl:
        raise SystemExit(f"missing observation validation marker: {token}")

for token in [
    '#include "NativeTimerObservation.h"',
    "VdrNativeTimerObservationMapStatus",
    "VdrNativeTimerObservationMapResult",
    "class VdrNativeTimerObservationMapper",
    "NativeTimerObservation observation",
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
    "nativeTimerObservedStateFingerprint(state)",
    "nativeTimerObservationValid(observation)",
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
    if forbidden in source + observation + observation_impl:
        raise SystemExit(f"premature Slice-11 observation boundary crossing: {forbidden}")

if "NativeTimerBinding.h" in header or "NativeTimerBinding" in header:
    raise SystemExit("VDR mapper must depend on NativeTimerObservation, not NativeTimerBinding")

for token in [
    'mapped.observation.backendGeneration == 7',
    'mapped.observation.backendNativeTimerId == "timer:42"',
    "nativeTimerObservationValid(mapped.observation)",
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
    "Backend-Neutral Native Timer Observation and VDR Mapper",
    "mapping/contract only",
    "NativeTimerObservation` lives under `core/timers`",
    "explicit backend/generation/time evidence",
    "VdrSnapshot",
    "no backend generation",
    "Provider-private fields stay below the boundary",
    "No repository write",
    "does not infer or set",
    "no `core/vdr` exception is required",
    "no installed runtime behavior",
    "backend-neutral readback application service",
]:
    if token not in doc:
        raise SystemExit(f"missing Slice-11 documentation marker: {token}")

for token in [
    "test-phase64-vdr-native-timer-observation-mapper-architecture",
    "test-phase64-vdr-native-timer-observation-mapper:",
    "core/timers/src/NativeTimerObservation.cpp",
    "core/vdr/src/VdrNativeTimerObservationMapper.cpp",
    "core/vdr/tests/test_vdr_native_timer_observation_mapper.cpp",
    "test-fast:",
    "test-architecture:",
]:
    if token not in make:
        raise SystemExit(f"missing Slice-11 test-graph marker: {token}")

print("Phase-64 VDR native Timer observation mapper check passed")
print(
    "Slice-11 boundary: backend-neutral present observation + explicit "
    "generation-fenced VDR mapping only; application/reconciliation/native mutation deferred")
