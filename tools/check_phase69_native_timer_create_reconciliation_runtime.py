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
            f"missing Phase-69.C reconciliation composition file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "daemon_h": [
        '#include "NativeTimerCreateReadbackVerificationService.h"',
        '#include "NativeTimerCreateOperationCompletionService.h"',
        "std::unique_ptr<vdrsuite::timers::NativeTimerCreateReadbackVerificationService>",
        "nativeTimerCreateReadbackVerificationService_;",
        "std::unique_ptr<vdrsuite::timers::NativeTimerCreateOperationCompletionService>",
        "nativeTimerCreateOperationCompletionService_;",
    ],
    "daemon_init": [
        "nativeTimerCreateReadbackVerificationService_ =",
        "vdrsuite::timers::NativeTimerCreateReadbackVerificationService",
        "*nativeTimerBindingRepository_",
        "nativeTimerCreateOperationCompletionService_ =",
        "vdrsuite::timers::NativeTimerCreateOperationCompletionService",
        "*mutationOperationRepository_",
        "*timerAssignmentRepository_",
    ],
    "daemon_shutdown": [
        "nativeTimerCreateOperationCompletionService_.reset();",
        "nativeTimerCreateReadbackVerificationService_.reset();",
    ],
    "daemon_sources": [
        "core/timers/src/NativeTimerObservation.cpp",
        "core/timers/src/NativeTimerCreateReadbackEvidence.cpp",
        "core/timers/src/NativeTimerCreateReadbackVerificationService.cpp",
        "core/timers/src/NativeTimerCreateOperationCompletionService.cpp",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing Timer CREATE reconciliation composition marker in {label}: {token}")

for member in [
    "std::unique_ptr<vdrsuite::timers::NativeTimerCreateReadbackVerificationService>",
    "std::unique_ptr<vdrsuite::timers::NativeTimerCreateOperationCompletionService>",
]:
    if contents["daemon_h"].count(member) != 1:
        raise SystemExit(f"DaemonRuntime must own exactly one reconciliation owner: {member}")

for source in required["daemon_sources"]:
    if contents["daemon_sources"].count(source) != 1:
        raise SystemExit(f"reconciliation source must be linked exactly once: {source}")

init = contents["daemon_init"]
binding_repo = init.find("nativeTimerBindingRepository_ =")
assignment_repo = init.find("timerAssignmentRepository_ =")
operation_repo = init.find("mutationOperationRepository_ =")
dispatch = init.find("nativeTimerCreateDispatchService_ =")
verification = init.find("nativeTimerCreateReadbackVerificationService_ =")
completion = init.find("nativeTimerCreateOperationCompletionService_ =")

if not (
    0 <= operation_repo < dispatch < verification < completion
    and 0 <= assignment_repo < completion
    and 0 <= binding_repo < verification < completion
):
    raise SystemExit(
        "reconciliation composition order must preserve repository -> dispatch -> verification -> completion dependencies")

shutdown = contents["daemon_shutdown"]
completion_reset = shutdown.find(
    "nativeTimerCreateOperationCompletionService_.reset();")
verification_reset = shutdown.find(
    "nativeTimerCreateReadbackVerificationService_.reset();")
dispatch_reset = shutdown.find("nativeTimerCreateDispatchService_.reset();")
binding_reset = shutdown.find("nativeTimerBindingRepository_.reset();")
assignment_reset = shutdown.find("timerAssignmentRepository_.reset();")
operation_reset = shutdown.find("mutationOperationRepository_.reset();")

if not (
    0 <= completion_reset < verification_reset < dispatch_reset
    and completion_reset < assignment_reset
    and verification_reset < binding_reset
    and completion_reset < operation_reset
):
    raise SystemExit(
        "reconciliation owners must stop before their dispatch/repository dependencies")

for label in ["public_h", "public_cpp", "security"]:
    for forbidden in [
        "NativeTimerCreateReadbackVerificationService",
        "NativeTimerCreateOperationCompletionService",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"Timer CREATE reconciliation authority leaked into public/security layer {label}: {forbidden}")

# This slice is composition only. A native effect must remain impossible until a
# later exact-candidate slice deliberately activates the existing handoff and
# wires its outcome/readback lifecycle.
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
        text = path.read_text(encoding="utf-8", errors="ignore")
        for forbidden_call in [
            "backendAgentNativeTimerCreateReservationService_->reserve(",
            "nativeTimerCreateDispatchService_->claimAfterReservation(",
            "backendAgentNativeTimerCreateActivationService_->activateDispatching(",
            "nativeTimerCreateDispatchService_->applyOutcome(",
            "nativeTimerCreateReadbackVerificationService_->verify(",
            "timerAssignmentFulfillmentService_->bindVerified(",
            "nativeTimerCreateOperationCompletionService_->complete(",
        ]:
            if forbidden_call in text:
                raise SystemExit(
                    "Timer CREATE reconciliation runtime must remain dormant in this slice: "
                    + str(path.relative_to(ROOT)) + " -> " + forbidden_call)

if "registerTimerCreateAdmission" not in contents["daemon_init"]:
    raise SystemExit("accepted public Timer CREATE admission callback disappeared")

print("Phase-69.C native Timer CREATE reconciliation runtime composition check passed")
print("Boundary: existing outcome/readback/completion owners are composed but native dispatch remains dormant")
