#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "daemon_shutdown": ROOT / "core/daemon/src/DaemonRuntimeShutdown.cpp",
    "daemon_sources": ROOT / "mk/daemon-sources.mk",
    "public_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "public_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
}

contents = {}
for label, path in paths.items():
    if not path.is_file():
        raise SystemExit(
            f"missing Phase-69.C CREATE fulfillment composition file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "daemon_h": [
        '#include "NativeTimerBindingRepository.h"',
        '#include "TimerAssignmentFulfillmentService.h"',
        "std::unique_ptr<vdrsuite::timers::NativeTimerBindingRepository>",
        "nativeTimerBindingRepository_;",
        "std::unique_ptr<vdrsuite::timers::TimerAssignmentFulfillmentService>",
        "timerAssignmentFulfillmentService_;",
    ],
    "daemon_init": [
        "nativeTimerBindingRepository_ =",
        "std::make_unique<vdrsuite::timers::NativeTimerBindingRepository>(",
        "nativeTimerBindingRepository_->ensureSchema()",
        "timerAssignmentFulfillmentService_ =",
        "std::make_unique<vdrsuite::timers::TimerAssignmentFulfillmentService>(",
        "*timerAssignmentRepository_",
        "*nativeTimerBindingRepository_",
    ],
    "daemon_shutdown": [
        "timerAssignmentFulfillmentService_.reset();",
        "nativeTimerBindingRepository_.reset();",
        "timerAssignmentRepository_.reset();",
    ],
    "daemon_sources": [
        "core/timers/src/NativeTimerBindingRepository.cpp",
        "core/timers/src/NativeTimerBindingReadRepository.cpp",
        "core/timers/src/NativeTimerBindingWriteRepository.cpp",
        "core/timers/src/TimerAssignmentFulfillmentService.cpp",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing CREATE fulfillment composition marker in {label}: {token}")

for member in [
    "std::unique_ptr<vdrsuite::timers::NativeTimerBindingRepository>",
    "std::unique_ptr<vdrsuite::timers::TimerAssignmentFulfillmentService>",
]:
    if contents["daemon_h"].count(member) != 1:
        raise SystemExit(f"DaemonRuntime must own exactly one: {member}")

for source in required["daemon_sources"]:
    if contents["daemon_sources"].count(source) != 1:
        raise SystemExit(f"daemon source must be linked exactly once: {source}")

init = contents["daemon_init"]
assignment_repo = init.find("timerAssignmentRepository_ =")
binding_repo = init.find("nativeTimerBindingRepository_ =")
fulfillment = init.find("timerAssignmentFulfillmentService_ =")
preparation = init.find("nativeTimerCreateOperationPreparationService_ =")
if not (0 <= assignment_repo < binding_repo < fulfillment < preparation):
    raise SystemExit(
        "CREATE lifecycle composition must be assignment repo -> binding repo -> fulfillment -> preparation")

shutdown = contents["daemon_shutdown"]
fulfillment_reset = shutdown.find("timerAssignmentFulfillmentService_.reset();")
binding_reset = shutdown.find("nativeTimerBindingRepository_.reset();")
assignment_reset = shutdown.find("timerAssignmentRepository_.reset();")
if not (0 <= fulfillment_reset < binding_reset < assignment_reset):
    raise SystemExit(
        "CREATE fulfillment must stop before binding and assignment repositories")

for label in ["public_h", "public_cpp"]:
    for forbidden in [
        "NativeTimerBindingRepository",
        "TimerAssignmentFulfillmentService",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"CREATE fulfillment authority leaked into public HTTP in {label}: {forbidden}")

for forbidden in [
    "NativeTimerBindingRepository",
    "TimerAssignmentFulfillmentService",
]:
    if forbidden in contents["security"]:
        raise SystemExit(
            f"CREATE fulfillment authority leaked into public security: {forbidden}")

for required_public_marker in [
    'const bool isPublicTimerAssignmentRead =',
    'const bool isPublicTimerAssignmentCreate =',
    'isPublicV1ReadOnlyMethodMismatch =',
    'isPublicTimerAssignmentResource;',
]:
    if required_public_marker not in contents["security"]:
        raise SystemExit(
            "reviewed public TimerAssignment read/create classification missing: "
            + required_public_marker)

for scan_root in [
    ROOT / "api",
    ROOT / "core" / "daemon",
    ROOT / "core" / "http",
    ROOT / "core" / "security",
]:
    if not scan_root.exists():
        continue
    for path in scan_root.rglob("*"):
        if not path.is_file() or path.suffix not in {
            ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".inc"
        }:
            continue
        text_value = path.read_text(encoding="utf-8", errors="ignore")
        for forbidden_call in [
            "timerAssignmentFulfillmentService_->beginProvisioning(",
            "timerAssignmentFulfillmentService_->bindVerified(",
        ]:
            if forbidden_call in text_value:
                raise SystemExit(
                    "CREATE fulfillment runtime must remain dormant in this slice: "
                    + str(path.relative_to(ROOT)) + " -> " + forbidden_call)

print("Phase-69.C native Timer CREATE fulfillment runtime composition check passed")
print("Boundary: accepted fulfillment owner remains the only transition authority; public HTTP cannot call it directly")
