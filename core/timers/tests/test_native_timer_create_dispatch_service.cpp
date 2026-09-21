#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

#include <cassert>
#include <string>

namespace
{
vdrsuite::timers::NativeTimerSpecification specification()
{
    vdrsuite::timers::NativeTimerSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 dispatch";
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1030";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

vdrsuite::timers::NativeTimerCreateOperationPayload createPayload()
{
    vdrsuite::timers::NativeTimerCreateOperationPayload payload;
    payload.timerAssignmentId = "ta_create_dispatch_1";
    payload.expectedAssignmentRevision = "2";
    payload.expectedIntentRevision = "5";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "ntb_create_dispatch_1";
    payload.backendId = "default";
    payload.backendGeneration = 7;
    payload.expectedSpecification = specification();
    return payload;
}

vdrsuite::operations::MutationOperation operation()
{
    using namespace vdrsuite::operations;
    MutationOperation value;
    value.operationId = "op_create_dispatch_1";
    value.idempotencyKey = "idem_create_dispatch_1";
    value.actorId = "system_phase64";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.resourceType = "TimerAssignment";
    value.resourceId = "ta_create_dispatch_1";
    value.expectedRevision = "2";
    value.expectedResourceFingerprint =
        vdrsuite::timers::nativeTimerSpecificationFingerprint(specification());
    value.actionFamily = "timer.create";
    value.requestFingerprint = "request_create_dispatch_1";
    value.requestedAt = 100;
    value.deadline = 500;
    value.verificationPolicy = MutationOperationVerificationPolicy::readbackRequired;
    value.state = MutationOperationState::accepted;
    value.updatedAt = 100;
    return value;
}

vdrsuite::timers::NativeTimerCreateDispatchClaimRequest request()
{
    using namespace vdrsuite::timers;
    NativeTimerCreateDispatchClaimRequest value;
    value.operationId = "op_create_dispatch_1";
    value.expectedOperationRevision = "1";
    value.timerAssignmentId = "ta_create_dispatch_1";
    value.nativeTimerBindingId = "ntb_create_dispatch_1";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(specification());
    value.reservation.commandId = "cmd_create_dispatch_1";
    value.reservation.requestFingerprint = "fp1_0123456789abcdef";
    return value;
}
}

int main()
{
    using namespace vdrsuite::operations;
    using namespace vdrsuite::timers;

    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository repository(database);
    assert(repository.ensureSchema());

    const auto payload = createPayload();
    MutationOperationPayload durablePayload;
    durablePayload.operationId = "op_create_dispatch_1";
    durablePayload.payloadType = "native.timer.create";
    durablePayload.payloadVersion = 1;
    durablePayload.payload = serializeNativeTimerCreateOperationPayload(payload);
    durablePayload.payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(payload);
    const auto reservedOperation =
        repository.reserveWithPayload(operation(), durablePayload);
    assert(reservedOperation.ok());
    assert(reservedOperation.operation.operationRevision == "1");

    const auto reference = request().reservation;
    const std::string serializedReference =
        serializeNativeTimerCreateCommandReservationReference(reference);
    assert(!serializedReference.empty());
    NativeTimerCreateCommandReservationReference parsedReference;
    assert(parseNativeTimerCreateCommandReservationReference(
        serializedReference, parsedReference));
    assert(parsedReference.commandId == reference.commandId);
    assert(parsedReference.requestFingerprint == reference.requestFingerprint);
    assert(!parseNativeTimerCreateCommandReservationReference(
        serializedReference + "x", parsedReference));

    NativeTimerCreateDispatchService service(repository);
    const auto claimed = service.claimAfterReservation(request(), 120);
    assert(claimed.status == NativeTimerCreateDispatchClaimStatus::claimed);
    assert(claimed.operation.state == MutationOperationState::dispatching);
    assert(claimed.operation.operationRevision == "2");
    assert(claimed.operation.resultReference == serializedReference);
    assert(claimed.reservation.commandId == reference.commandId);

    // Restart/replay must recover the same durable reservation reference. It
    // must never accept a fresh command identity for the same CREATE.
    const auto replay = service.claimAfterReservation(request(), 121);
    assert(replay.status == NativeTimerCreateDispatchClaimStatus::alreadyClaimed);
    assert(replay.operation.operationRevision == "2");
    assert(replay.reservation.commandId == reference.commandId);

    auto differentReservation = request();
    differentReservation.reservation.commandId = "cmd_create_dispatch_other";
    differentReservation.reservation.requestFingerprint = "fp1_fedcba9876543210";
    const auto reservationConflict =
        service.claimAfterReservation(differentReservation, 122);
    assert(reservationConflict.status ==
        NativeTimerCreateDispatchClaimStatus::identityConflict);

    auto wrongBinding = request();
    wrongBinding.nativeTimerBindingId = "ntb_create_dispatch_other";
    const auto bindingConflict = service.claimAfterReservation(wrongBinding, 123);
    assert(bindingConflict.status ==
        NativeTimerCreateDispatchClaimStatus::identityConflict);

    // A fresh operation cannot enter dispatching with a stale operation revision.
    auto secondOperation = operation();
    secondOperation.operationId = "op_create_dispatch_2";
    secondOperation.idempotencyKey = "idem_create_dispatch_2";
    auto secondPayload = payload;
    MutationOperationPayload secondDurable = durablePayload;
    secondDurable.operationId = secondOperation.operationId;
    secondDurable.payload = serializeNativeTimerCreateOperationPayload(secondPayload);
    secondDurable.payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(secondPayload);
    assert(repository.reserveWithPayload(secondOperation, secondDurable).ok());

    auto staleRevision = request();
    staleRevision.operationId = secondOperation.operationId;
    staleRevision.expectedOperationRevision = "9";
    staleRevision.reservation.commandId = "cmd_create_dispatch_2";
    staleRevision.reservation.requestFingerprint = "fp1_1111111111111111";
    const auto revisionConflict =
        service.claimAfterReservation(staleRevision, 124);
    assert(revisionConflict.status ==
        NativeTimerCreateDispatchClaimStatus::operationRevisionConflict);

    // Deadline is enforced before first dispatch claim, but never retroactively
    // invalidates an already-dispatching operation on restart.
    auto expiredOperation = operation();
    expiredOperation.operationId = "op_create_dispatch_expired";
    expiredOperation.idempotencyKey = "idem_create_dispatch_expired";
    expiredOperation.deadline = 110;
    MutationOperationPayload expiredPayload = durablePayload;
    expiredPayload.operationId = expiredOperation.operationId;
    assert(repository.reserveWithPayload(expiredOperation, expiredPayload).ok());
    auto expiredRequest = request();
    expiredRequest.operationId = expiredOperation.operationId;
    expiredRequest.reservation.commandId = "cmd_create_dispatch_expired";
    expiredRequest.reservation.requestFingerprint = "fp1_2222222222222222";
    const auto expired = service.claimAfterReservation(expiredRequest, 120);
    assert(expired.status == NativeTimerCreateDispatchClaimStatus::deadlineExpired);

    return 0;
}
