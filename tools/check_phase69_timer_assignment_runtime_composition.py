#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "daemon_build": ROOT / "mk/runtime-api-tests.mk",
    "intent_guard": ROOT / "tools/check_phase64_timer_intent_contract.py",
    "repository_guard": ROOT / "tools/check_phase64_timer_assignment_repository.py",
    "test": ROOT / "api/rest/tests/test_public_timer_assignment_lookup.cpp",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(f"missing Phase-69.C runtime composition file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "public_h": [
        "PublicTimerAssignmentLookupStatus",
        "PublicTimerAssignmentRevisionResource",
        "registerTimerAssignmentLookup",
        "resetTimerAssignmentLookup",
        "timerAssignmentLookupConfigured",
        "lookupTimerAssignment",
    ],
    "public_cpp": [
        "PublicApiRuntime::registerTimerAssignmentLookup",
        "PublicApiRuntime::resetTimerAssignmentLookup",
        "PublicApiRuntime::lookupTimerAssignment",
    ],
    "daemon_h": [
        "TimerAssignmentReadService.h",
        "TimerAssignmentRepository.h",
        "timerAssignmentRepository_",
        "timerAssignmentReadService_",
    ],
    "daemon_init": [
        "std::make_unique<vdrsuite::timers::TimerAssignmentRepository>",
        "timerAssignmentRepository_->ensureSchema()",
        "std::make_unique<vdrsuite::timers::TimerAssignmentReadService>",
        "registerTimerAssignmentLookup",
        "timerAssignmentReadService_->findForBackend",
        "found.assignment.assignmentRevision",
    ],
    "daemon_shutdown": [
        "resetTimerAssignmentLookup()",
        "timerAssignmentReadService_.reset()",
        "timerAssignmentRepository_.reset()",
    ],
    "daemon_sources": [
        "core/timers/src/TimerAssignment.cpp",
        "core/timers/src/TimerAssignmentRepository.cpp",
        "core/timers/src/TimerAssignmentReadService.cpp",
    ],
    "daemon_build": [
        "daemon:",
        "-Icore/timers/include",
    ],
    "test": [
        "runtime.resetTimerAssignmentLookup()",
        "runtime.registerTimerAssignmentLookup(",
        "runtime.lookupTimerAssignment(",
        '"/api/v1/timer-assignments/assignment:one?backend=backend:one"',
        "routeStillClosed.statusCode == 404",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Phase-69.C runtime composition marker in {label}: {token}")

for path in [
    "api/rest/include/PublicApiRuntime.h",
    "api/rest/src/PublicApiRuntime.cpp",
    "api/rest/tests/test_public_timer_assignment_lookup.cpp",
    "core/daemon/include/DaemonRuntime.h",
    "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "core/daemon/src/DaemonRuntimeShutdown.cpp",
]:
    marker = f'Path("{path}")'
    if marker not in contents["intent_guard"]:
        raise SystemExit(
            f"Phase-64 Timer guard missing exact reviewed runtime path: {path}")

for path in [
    "core/daemon/include/DaemonRuntime.h",
    "core/daemon/src/DaemonRuntimeInitialization.cpp",
]:
    marker = f'Path("{path}")'
    if marker not in contents["repository_guard"]:
        raise SystemExit(
            f"TimerAssignment repository guard missing reviewed runtime path: {path}")

route_literal = '"/api/v1/timer-assignments/'
if route_literal in contents["public_cpp"]:
    raise SystemExit(
        "TimerAssignment public HTTP route opened prematurely in runtime-composition slice")

if "Idempotency-Key" in contents["public_h"]:
    raise SystemExit(
        "runtime-composition slice must not add public idempotency request handling")

print("Phase-69.C TimerAssignment runtime composition check passed")
print("Boundary: one daemon repository/read service + dormant public lookup; HTTP route closed")
