#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "phase64_guard": ROOT / "tools/check_phase64_native_timer_create_operation_preparation.py",
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C CREATE preparation composition file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "daemon_h": [
        '#include "TimerIntentRepository.h"',
        '#include "NativeTimerCreateOperationPreparationService.h"',
        "std::unique_ptr<vdrsuite::timers::TimerIntentRepository>",
        "timerIntentRepository_;",
        "std::unique_ptr<vdrsuite::timers::NativeTimerCreateOperationPreparationService>",
        "nativeTimerCreateOperationPreparationService_;",
    ],
    "daemon_init": [
        "timerIntentRepository_ =",
        "std::make_unique<vdrsuite::timers::TimerIntentRepository>(",
        "timerIntentRepository_->ensureSchema()",
        "nativeTimerCreateOperationPreparationService_ =",
        "NativeTimerCreateOperationPreparationService>(",
        "*timerIntentRepository_",
        "*timerAssignmentRepository_",
        "*mutationOperationRepository_",
    ],
    "daemon_shutdown": [
        "nativeTimerCreateOperationPreparationService_.reset();",
        "timerAssignmentRepository_.reset();",
        "timerIntentRepository_.reset();",
        "mutationOperationRepository_.reset();",
    ],
    "daemon_sources": [
        "core/timers/src/TimerIntent.cpp",
        "core/timers/src/TimerIntentRepository.cpp",
        "core/timers/src/NativeTimerBinding.cpp",
        "core/timers/src/NativeTimerSpecification.cpp",
        "core/timers/src/NativeTimerCreateOperationPayload.cpp",
        "core/timers/src/NativeTimerCreateOperationPreparationService.cpp",
    ],
    "phase64_guard": [
        'Path("core/daemon/include/DaemonRuntime.h")',
        'Path("core/daemon/src/DaemonRuntimeInitialization.cpp")',
        'Path("core/daemon/src/DaemonRuntimeShutdown.cpp")',
        "unreviewed CREATE preparation runtime wiring",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing CREATE preparation composition marker in {label}: {token}")

if contents["daemon_h"].count(
    "std::unique_ptr<vdrsuite::timers::TimerIntentRepository>") != 1:
    raise SystemExit("DaemonRuntime must own exactly one TimerIntentRepository")

if contents["daemon_h"].count(
    "std::unique_ptr<vdrsuite::timers::NativeTimerCreateOperationPreparationService>") != 1:
    raise SystemExit(
        "DaemonRuntime must own exactly one NativeTimerCreateOperationPreparationService")

for source in [
    "core/timers/src/TimerIntent.cpp",
    "core/timers/src/TimerIntentRepository.cpp",
    "core/timers/src/NativeTimerBinding.cpp",
    "core/timers/src/NativeTimerSpecification.cpp",
    "core/timers/src/NativeTimerCreateOperationPayload.cpp",
    "core/timers/src/NativeTimerCreateOperationPreparationService.cpp",
]:
    if contents["daemon_sources"].count(source) != 1:
        raise SystemExit(f"daemon source must be linked exactly once: {source}")

intent_pos = contents["daemon_init"].find("timerIntentRepository_ =")
assignment_pos = contents["daemon_init"].find("timerAssignmentRepository_ =")
preparation_pos = contents["daemon_init"].find(
    "nativeTimerCreateOperationPreparationService_ =")
if not (0 <= intent_pos < assignment_pos < preparation_pos):
    raise SystemExit(
        "CREATE preparation initialization order must be intent -> assignment -> service")

shutdown = contents["daemon_shutdown"]
preparation_reset = shutdown.find(
    "nativeTimerCreateOperationPreparationService_.reset();")
assignment_reset = shutdown.find("timerAssignmentRepository_.reset();")
intent_reset = shutdown.find("timerIntentRepository_.reset();")
operation_reset = shutdown.find("mutationOperationRepository_.reset();")
if not (
    0 <= preparation_reset < assignment_reset < intent_reset < operation_reset
):
    raise SystemExit(
        "CREATE preparation service must stop before its repository dependencies")

# The public mutation successor may now invoke this already accepted owner.
# Keep this guard responsible for ownership/composition and require the focused
# successor guard before public mutation semantics are allowed.
successor_guard = ROOT / "tools/check_phase69_public_timer_create_submission.py"
if not successor_guard.is_file():
    raise SystemExit("public Timer CREATE successor guard is missing")
successor = successor_guard.read_text(encoding="utf-8")
for marker in [
    "preparationService_.prepare(preparation)",
    "findByIdempotencyScope(",
    "accepted Timer CREATE replay must recover durable payload",
]:
    if marker not in successor:
        raise SystemExit(
            "public Timer CREATE successor guard missing preparation marker: "
            + marker)

print("Phase-69.C native Timer CREATE preparation runtime composition check passed")
print("Boundary: one TimerIntent repository + one preparation authority; invocation owned by guarded public successor")
