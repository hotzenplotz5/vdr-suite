#!/usr/bin/env python3

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "database_h": ROOT / "core/sqlite/include/Database.h",
    "database_cpp": ROOT / "core/sqlite/src/Database.cpp",
    "assignment_h": ROOT / "core/timers/include/TimerAssignmentRepository.h",
    "assignment_cpp": ROOT / "core/timers/src/TimerAssignmentRepository.cpp",
    "operation_h": ROOT / "core/operations/include/MutationOperationRepository.h",
    "operation_cpp": ROOT / "core/operations/src/MutationOperationRepository.cpp",
    "fulfillment_h": ROOT / "core/timers/include/TimerAssignmentFulfillmentService.h",
    "fulfillment_cpp": ROOT / "core/timers/src/TimerAssignmentFulfillmentService.cpp",
    "preparation_h": ROOT / "core/timers/include/NativeTimerCreateOperationPreparationService.h",
    "preparation_cpp": ROOT / "core/timers/src/NativeTimerCreateOperationPreparationService.cpp",
    "admission_h": ROOT / "core/timers/include/NativeTimerCreateAdmissionService.h",
    "admission_cpp": ROOT / "core/timers/src/NativeTimerCreateAdmissionService.cpp",
    "test": ROOT / "core/timers/tests/test_native_timer_create_admission_service.cpp",
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
            f"missing Phase-69.C atomic admission file: {path.relative_to(ROOT)}")
    contents[label] = path.read_text(encoding="utf-8")

required = {
    "database_h": ["bool transactionActive() const;"],
    "database_cpp": ["sqlite3_get_autocommit(db_) == 0"],
    "assignment_h": ["updateInCurrentTransaction"],
    "assignment_cpp": [
        "updateInCurrentTransactionInternal",
        "database.transactionActive()",
        "TimerAssignmentRepository::updateInCurrentTransaction",
    ],
    "operation_h": ["reserveWithPayloadInCurrentTransaction"],
    "operation_cpp": [
        "reserveInCurrentTransactionInternal",
        "database.transactionActive()",
        "MutationOperationRepository::reserveWithPayloadInCurrentTransaction",
    ],
    "fulfillment_h": ["beginProvisioningInCurrentTransaction"],
    "fulfillment_cpp": [
        "beginProvisioningImpl",
        "assignmentRepository_.updateInCurrentTransaction",
    ],
    "preparation_h": ["prepareInCurrentTransaction"],
    "preparation_cpp": [
        "prepareImpl",
        "operationRepository_.reserveWithPayloadInCurrentTransaction",
    ],
    "admission_h": [
        "NativeTimerCreateAdmissionService",
        "NativeTimerCreateAdmissionRequest",
        "idempotencyKey",
        "expectedAssignmentRevision",
    ],
    "admission_cpp": [
        "BEGIN IMMEDIATE TRANSACTION;",
        "findByIdempotencyScope",
        "generateMutationOperationId",
        "generateNativeTimerBindingId",
        "beginProvisioningInCurrentTransaction",
        "prepareInCurrentTransaction",
        'database_.execute("COMMIT;")',
        'database_.execute("ROLLBACK;")',
        '"native-timer-create-admission/1|"',
    ],
    "test": [
        "fail_atomic_timer_create_operation",
        "RAISE(ABORT,'forced admission failure')",
        "TimerAssignmentState::selected",
        "MutationOperationRepositoryStatus::notFound",
        "NativeTimerCreateAdmissionStatus::replayed",
        "NativeTimerCreateAdmissionStatus::idempotencyConflict",
        'assignmentRevision == "2"',
    ],
    "daemon_h": [
        '#include "NativeTimerCreateAdmissionService.h"',
        "std::unique_ptr<vdrsuite::timers::NativeTimerCreateAdmissionService>",
        "nativeTimerCreateAdmissionService_;",
    ],
    "daemon_init": [
        "nativeTimerCreateAdmissionService_ =",
        "vdrsuite::timers::NativeTimerCreateAdmissionService>",
        "*timerAssignmentRepository_",
        "*mutationOperationRepository_",
        "*timerAssignmentFulfillmentService_",
        "*nativeTimerCreateOperationPreparationService_",
    ],
    "daemon_shutdown": [
        "nativeTimerCreateAdmissionService_.reset();",
        "nativeTimerCreateOperationPreparationService_.reset();",
        "timerAssignmentFulfillmentService_.reset();",
    ],
    "daemon_sources": [
        "core/timers/src/NativeTimerCreateAdmissionService.cpp",
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            raise SystemExit(
                f"missing atomic admission marker in {label}: {token}")

if contents["daemon_h"].count(
    "std::unique_ptr<vdrsuite::timers::NativeTimerCreateAdmissionService>") != 1:
    raise SystemExit("DaemonRuntime must own exactly one atomic CREATE admission service")

if contents["daemon_sources"].count(
    "core/timers/src/NativeTimerCreateAdmissionService.cpp") != 1:
    raise SystemExit("atomic CREATE admission source must be linked exactly once")

admission = contents["admission_cpp"]
order = [
    admission.find("findByIdempotencyScope"),
    admission.find('database_.execute("BEGIN IMMEDIATE TRANSACTION;")'),
    admission.find("assignmentRepository_.findById"),
    admission.find("generateMutationOperationId"),
    admission.find("beginProvisioningInCurrentTransaction"),
    admission.find("prepareInCurrentTransaction"),
    admission.find('database_.execute("COMMIT;")'),
]
if any(position < 0 for position in order) or order != sorted(order):
    raise SystemExit(
        "atomic admission order must be replay lookup -> BEGIN -> assignment "
        "reload -> IDs -> provisioning -> preparation -> COMMIT")

for forbidden in [
    "BackendAgentNativeTimerCreateReservationService",
    "BackendAgentCommandReservationRepository",
    "NativeTimerCreateDispatchService",
    "BackendAgentNativeTimerCreateActivationService",
    "claimAfterReservation",
    "activateDispatching",
]:
    if forbidden in admission:
        raise SystemExit(
            f"atomic admission acquired post-admission/native execution authority: {forbidden}")

for label in ["public_h", "public_cpp"]:
    for forbidden in [
        "NativeTimerCreateAdmissionService",
        "Idempotency-Key",
        "timers.create",
        "timer.create",
        "isPublicTimerAssignmentCreate",
        "publicTimerAssignmentCreate",
    ]:
        if forbidden in contents[label]:
            raise SystemExit(
                f"atomic admission opened public mutation semantics in {label}: {forbidden}")

for forbidden in [
    "NativeTimerCreateAdmissionService",
    "Idempotency-Key",
    "isPublicTimerAssignmentCreate",
    "publicTimerAssignmentCreate",
]:
    if forbidden in contents["security"]:
        raise SystemExit(
            f"atomic admission opened public mutation security: {forbidden}")

for required_read_only_marker in [
    'const bool isPublicTimerAssignmentRead =',
    'request.method == "GET" &&',
    'isPublicV1ReadOnlyMethodMismatch =',
    'isPublicTimerAssignmentResource);',
]:
    if required_read_only_marker not in contents["security"]:
        raise SystemExit(
            "public TimerAssignment boundary is no longer read-only: "
            + required_read_only_marker)

for label in ["assignment_cpp", "operation_cpp"]:
    if "BEGIN IMMEDIATE TRANSACTION;" not in contents[label]:
        raise SystemExit(
            f"normal repository transaction ownership disappeared from {label}")

print("Phase-69.C atomic Timer CREATE admission check passed")
print("Boundary: selected->provisioning and operation+payload reserve share one commit; public/native dispatch remain closed")
