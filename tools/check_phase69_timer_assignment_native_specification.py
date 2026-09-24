#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "assignment_h": ROOT / "core/timers/include/TimerAssignment.h",
    "repository": ROOT / "core/timers/src/TimerAssignmentRepository.cpp",
    "codec_h": ROOT / "core/timers/include/TimerAssignmentDesiredNativeTimerSpecification.h",
    "codec": ROOT / "core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp",
    "planner_h": ROOT / "core/timers/include/TimerAssignmentPlanner.h",
    "planner": ROOT / "core/timers/src/TimerAssignmentPlanner.cpp",
    "scheduling": ROOT / "core/timers/src/TimerAssignmentSchedulingService.cpp",
    "reassignment": ROOT / "core/timers/src/TimerAssignmentReassignmentService.cpp",
    "migration_test": ROOT / "core/timers/tests/test_timer_assignment_desired_native_specification.cpp",
    "planner_test": ROOT / "core/timers/tests/test_timer_assignment_planner.cpp",
    "scheduling_test": ROOT / "core/timers/tests/test_timer_assignment_scheduling_service.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C assignment specification file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "assignment_h": [
        '#include "NativeTimerSpecification.h"',
        "desiredNativeTimerSpecificationPresent",
        "NativeTimerSpecification desiredNativeTimerSpecification",
    ],
    "repository": [
        "desired_native_timer_specification",
        "ALTER TABLE timer_assignments",
        "TimerAssignmentDesiredNativeTimerSpecification.h",
        "desiredNativeTimerSpecificationValidForAssignment",
        "desiredNativeTimerSpecificationImmutable",
        "parseTimerAssignmentDesiredNativeTimerSpecification",
        "serializeTimerAssignmentDesiredNativeTimerSpecification",
    ],
    "codec": [
        '"timer-assignment-native-specification/1|"',
        "nativeTimerSpecificationValid",
        "timerAssignmentDesiredNativeTimerSpecificationEquivalent",
    ],
    "planner_h": [
        "desiredNativeTimerSpecificationPresent",
        "selectedNativeTimerSpecificationPresent",
    ],
    "planner": [
        '"native_timer_specification_missing"',
        '"native_timer_specification_invalid"',
        '"native_timer_specification_channel_mismatch"',
        "selectedNativeTimerSpecificationPresent =",
        "selectedNativeTimerSpecification =",
    ],
    "scheduling": [
        "assignment.desiredNativeTimerSpecificationPresent =",
        "decision.selectedNativeTimerSpecificationPresent",
        "assignment.desiredNativeTimerSpecification =",
    ],
    "reassignment": [
        "replacement.desiredNativeTimerSpecificationPresent =",
        "decision.selectedNativeTimerSpecificationPresent",
        "replacement.desiredNativeTimerSpecification =",
    ],
    "migration_test": [
        "installLegacySchema",
        "assignment:legacy",
        "desiredNativeTimerSpecificationPresent",
        "TimerAssignmentRepositoryStatus::invalid",
        "test_timer_assignment_desired_native_specification passed",
    ],
    "planner_test": [
        "native_timer_specification_missing",
        "native_timer_specification_channel_mismatch",
        "selectedNativeTimerSpecificationPresent",
    ],
    "scheduling_test": [
        "durableFirst",
        "desiredNativeTimerSpecificationPresent",
        "desiredNativeTimerSpecification.title",
    ],
    "daemon_sources": [
        "core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp",
    ],
}

for label, markers in required.items():
    for marker in markers:
        if marker not in contents[label]:
            raise SystemExit(
                f"missing assignment native specification marker in {label}: {marker}")

if contents["daemon_sources"].count(
    "core/timers/src/TimerAssignmentDesiredNativeTimerSpecification.cpp") != 1:
    raise SystemExit(
        "daemon must link assignment desired native specification codec exactly once")

# The public v1 TimerAssignment representation must remain backend-neutral.
for label in ["public_h", "public_cpp"]:
    for forbidden in [
        "NativeTimerSpecification",
        "desiredNativeTimerSpecification",
        "desired_native_timer_specification",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"public v1 exposed internal native Timer specification in {label}: {forbidden}")

# This prerequisite must not open the public mutation/security branch.
for forbidden in [
    "publicTimerAssignmentCreate",
    "isPublicTimerAssignmentCreate",
    "Idempotency-Key",
]:
    if forbidden in contents["security"]:
        raise SystemExit(
            f"assignment specification prerequisite opened public mutation security: {forbidden}")

print("Phase-69.C TimerAssignment desired native specification check passed")
print("Boundary: durable internal specification for new scheduler decisions; legacy rows remain readable; public v1 unchanged")
