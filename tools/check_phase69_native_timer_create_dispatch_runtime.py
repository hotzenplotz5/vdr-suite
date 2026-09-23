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
            f"missing Phase-69.C CREATE dispatch composition file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "daemon_h": [
        '#include "BackendAgentCommandReservation.h"',
        '#include "BackendAgentNativeTimerCreateReservation.h"',
        '#include "BackendAgentNativeTimerCreateActivation.h"',
        '#include "NativeTimerCreateDispatchService.h"',
        "std::unique_ptr<BackendAgentCommandReservationRepository>",
        "backendAgentCommandReservationRepository_;",
        "BackendAgentNativeTimerCreateReservationService>",
        "backendAgentNativeTimerCreateReservationService_;",
        "NativeTimerCreateDispatchService>",
        "nativeTimerCreateDispatchService_;",
        "BackendAgentNativeTimerCreateActivationService>",
        "backendAgentNativeTimerCreateActivationService_;",
    ],
    "daemon_init": [
        "backendAgentCommandReservationRepository_ =",
        "std::make_unique<BackendAgentCommandReservationRepository>(database_)",
        "backendAgentCommandReservationRepository_->ensureSchema()",
        "backendAgentNativeTimerCreateReservationService_ =",
        "*backendAgentCommandRepository_",
        "*backendAgentCommandReservationRepository_",
        "*backendAgentRepository_",
        "nativeTimerCreateDispatchService_ =",
        "*mutationOperationRepository_",
        "backendAgentNativeTimerCreateActivationService_ =",
    ],
    "daemon_shutdown": [
        "backendAgentNativeTimerCreateActivationService_.reset();",
        "backendAgentNativeTimerCreateReservationService_.reset();",
        "backendAgentCommandReservationRepository_.reset();",
        "nativeTimerCreateDispatchService_.reset();",
    ],
    "daemon_sources": [
        "core/timers/src/NativeTimerCreateReadbackExpectation.cpp",
        "core/timers/src/NativeTimerCreateDispatchService.cpp",
        "core/agent/src/BackendAgentCommandReservation.cpp",
        "core/agent/src/BackendAgentNativeTimerCreateReservation.cpp",
        "core/agent/src/BackendAgentNativeTimerCreateActivation.cpp",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing CREATE dispatch composition marker in {label}: {token}")

unique_members = [
    "std::unique_ptr<BackendAgentCommandReservationRepository>",
    "std::unique_ptr<vdrsuite::agent::BackendAgentNativeTimerCreateReservationService>",
    "std::unique_ptr<vdrsuite::timers::NativeTimerCreateDispatchService>",
    "std::unique_ptr<vdrsuite::agent::BackendAgentNativeTimerCreateActivationService>",
]
for member in unique_members:
    if contents["daemon_h"].count(member) != 1:
        raise SystemExit(f"DaemonRuntime must own exactly one: {member}")

for source in required["daemon_sources"]:
    if contents["daemon_sources"].count(source) != 1:
        raise SystemExit(f"daemon source must be linked exactly once: {source}")

init = contents["daemon_init"]
command_repo = init.find("backendAgentCommandRepository_ =")
reservation_repo = init.find("backendAgentCommandReservationRepository_ =")
reservation_service = init.find("backendAgentNativeTimerCreateReservationService_ =")
activation_service = init.find("backendAgentNativeTimerCreateActivationService_ =")
if not (
    0 <= command_repo < reservation_repo < reservation_service < activation_service
):
    raise SystemExit(
        "CREATE Agent composition order must be command repo -> reservation repo -> reservation -> activation")

dispatch_service = init.find("nativeTimerCreateDispatchService_ =")
preparation_service = init.find("nativeTimerCreateOperationPreparationService_ =")
if not (0 <= preparation_service < dispatch_service):
    raise SystemExit(
        "CREATE dispatch service must be composed after the preparation owner")

shutdown = contents["daemon_shutdown"]
activation_reset = shutdown.find(
    "backendAgentNativeTimerCreateActivationService_.reset();")
reservation_service_reset = shutdown.find(
    "backendAgentNativeTimerCreateReservationService_.reset();")
reservation_repo_reset = shutdown.find(
    "backendAgentCommandReservationRepository_.reset();")
agent_repo_reset = shutdown.find("backendAgentRepository_.reset();")
if not (
    0 <= activation_reset < reservation_service_reset < reservation_repo_reset < agent_repo_reset
):
    raise SystemExit(
        "CREATE Agent services/repository must stop before BackendAgentRepository")

dispatch_reset = shutdown.find("nativeTimerCreateDispatchService_.reset();")
operation_reset = shutdown.find("mutationOperationRepository_.reset();")
if not (0 <= dispatch_reset < operation_reset):
    raise SystemExit(
        "CREATE dispatch service must stop before MutationOperationRepository")

# The public mutation successor may now invoke the accepted durable chain.
# This guard continues to own the exact repository/service composition and
# delegates admission/idempotency/no-fallback rules to the focused successor.
successor_guard = ROOT / "tools/check_phase69_public_timer_create_submission.py"
if not successor_guard.is_file():
    raise SystemExit("public Timer CREATE successor guard is missing")
successor = successor_guard.read_text(encoding="utf-8")
for marker in [
    "reservationService_.reserve(",
    "dispatchService_.claimAfterReservation(",
    "activationService_.activateDispatching(",
    "Timer CREATE orchestration must remain prepare -> reserve -> claim -> activate",
]:
    if marker not in successor:
        raise SystemExit(
            "public Timer CREATE successor guard missing dispatch marker: "
            + marker)

print("Phase-69.C native Timer CREATE dispatch runtime composition check passed")
print("Boundary: durable reservation/dispatch/activation owners remain singular; invocation owned by guarded public successor")
