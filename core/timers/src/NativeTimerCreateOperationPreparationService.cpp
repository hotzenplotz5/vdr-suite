#include "NativeTimerCreateOperationPreparationService.h"

#include "MutationOperationRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cstddef>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxRequestFingerprintLength = 512;
constexpr const char* kPayloadType = "native.timer.create";
constexpr std::uint32_t kPayloadVersion = 1;

bool validIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool validRequest(const NativeTimerCreateOperationPreparationRequest& request)
{
    return validIdentity(request.operationId)
        && validIdentity(request.idempotencyKey)
        && validIdentity(request.actorId)
        && !request.requestFingerprint.empty()
        && request.requestFingerprint.size() <= kMaxRequestFingerprintLength
        && validIdentity(request.timerAssignmentId)
        && validIdentity(request.expectedAssignmentRevision)
        && validIdentity(request.expectedIntentRevision)
        && request.expectedAssignmentEpoch > 0
        && validIdentity(request.nativeTimerBindingId)
        && validIdentity(request.expectedBackendId)
        && request.expectedBackendGeneration > 0
        && nativeTimerSpecificationValid(request.expectedSpecification)
        && request.requestedAt > 0
        && (request.deadline == 0 || request.deadline >= request.requestedAt);
}

NativeTimerCreateOperationPreparationResult result(
    NativeTimerCreateOperationPreparationStatus status,
    const vdrsuite::operations::MutationOperation& operation = {},
    const NativeTimerCreateOperationPayload& payload = {})
{
    NativeTimerCreateOperationPreparationResult value;
    value.status = status;
    value.operation = operation;
    value.payload = payload;
    return value;
}
}

NativeTimerCreateOperationPreparationService::
NativeTimerCreateOperationPreparationService(
    TimerIntentRepository& intentRepository,
    TimerAssignmentRepository& assignmentRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository),
      operationRepository_(operationRepository)
{
}

NativeTimerCreateOperationPreparationResult
NativeTimerCreateOperationPreparationService::prepare(
    const NativeTimerCreateOperationPreparationRequest& request)
{
    using namespace vdrsuite::operations;

    if (!validRequest(request))
        return result(NativeTimerCreateOperationPreparationStatus::invalid);

    const auto assignmentResult =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (assignmentResult.status == TimerAssignmentRepositoryStatus::notFound)
        return result(
            NativeTimerCreateOperationPreparationStatus::assignmentNotFound);
    if (!assignmentResult.ok())
        return result(
            NativeTimerCreateOperationPreparationStatus::operationRepositoryError);

    const TimerAssignment& assignment = assignmentResult.assignment;

    const auto intentResult = intentRepository_.findById(assignment.timerIntentId);
    if (intentResult.status == TimerIntentRepositoryStatus::notFound)
        return result(NativeTimerCreateOperationPreparationStatus::intentNotFound);
    if (!intentResult.ok())
        return result(
            NativeTimerCreateOperationPreparationStatus::operationRepositoryError);

    const TimerIntent& intent = intentResult.intent;

    if (intent.intentRevision != request.expectedIntentRevision
        || assignment.intentRevision != request.expectedIntentRevision)
    {
        return result(
            NativeTimerCreateOperationPreparationStatus::intentRevisionConflict);
    }
    if (!timerIntentAssignable(intent.state))
        return result(
            NativeTimerCreateOperationPreparationStatus::intentStateConflict);
    if (assignment.assignmentRevision != request.expectedAssignmentRevision)
        return result(
            NativeTimerCreateOperationPreparationStatus::assignmentRevisionConflict);
    if (assignment.assignmentEpoch != request.expectedAssignmentEpoch)
        return result(
            NativeTimerCreateOperationPreparationStatus::assignmentEpochConflict);
    if (assignment.state != TimerAssignmentState::provisioning)
        return result(
            NativeTimerCreateOperationPreparationStatus::assignmentStateConflict);
    if (!assignment.nativeTimerBindingId.empty())
        return result(
            NativeTimerCreateOperationPreparationStatus::bindingAlreadyPresent);
    if (assignment.backendId != request.expectedBackendId)
        return result(NativeTimerCreateOperationPreparationStatus::backendConflict);
    if (assignment.backendGeneration != request.expectedBackendGeneration)
        return result(
            NativeTimerCreateOperationPreparationStatus::generationConflict);
    if (assignment.channelBinding.backendChannelId
        != request.expectedSpecification.channelId)
    {
        return result(NativeTimerCreateOperationPreparationStatus::channelConflict);
    }

    NativeTimerCreateOperationPayload createPayload;
    createPayload.timerAssignmentId = assignment.timerAssignmentId;
    createPayload.expectedAssignmentRevision = assignment.assignmentRevision;
    createPayload.expectedIntentRevision = intent.intentRevision;
    createPayload.assignmentEpoch = assignment.assignmentEpoch;
    createPayload.nativeTimerBindingId = request.nativeTimerBindingId;
    createPayload.backendId = assignment.backendId;
    createPayload.backendGeneration = assignment.backendGeneration;
    createPayload.expectedSpecification = request.expectedSpecification;

    const std::string serialized =
        serializeNativeTimerCreateOperationPayload(createPayload);
    const std::string payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(createPayload);
    const std::string specificationFingerprint =
        nativeTimerSpecificationFingerprint(request.expectedSpecification);
    if (serialized.empty()
        || payloadFingerprint.empty()
        || specificationFingerprint.empty())
    {
        return result(NativeTimerCreateOperationPreparationStatus::invalid);
    }

    MutationOperation operation;
    operation.operationId = request.operationId;
    operation.idempotencyKey = request.idempotencyKey;
    operation.actorId = request.actorId;
    operation.backendId = assignment.backendId;
    operation.backendGeneration = assignment.backendGeneration;
    operation.resourceType = "TimerAssignment";
    operation.resourceId = assignment.timerAssignmentId;
    operation.expectedRevision = assignment.assignmentRevision;
    operation.expectedResourceFingerprint = specificationFingerprint;
    operation.actionFamily = "timer.create";
    operation.requestFingerprint = request.requestFingerprint;
    operation.requestedAt = request.requestedAt;
    operation.deadline = request.deadline;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = request.requestedAt;

    MutationOperationPayload durablePayload;
    durablePayload.operationId = request.operationId;
    durablePayload.payloadType = kPayloadType;
    durablePayload.payloadVersion = kPayloadVersion;
    durablePayload.payload = serialized;
    durablePayload.payloadFingerprint = payloadFingerprint;

    const auto reserved =
        operationRepository_.reserveWithPayload(operation, durablePayload);
    switch (reserved.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(
                NativeTimerCreateOperationPreparationStatus::prepared,
                reserved.operation,
                createPayload);
        case MutationOperationRepositoryStatus::idempotentReplay:
            if (reserved.operation.state != MutationOperationState::accepted)
            {
                return result(
                    NativeTimerCreateOperationPreparationStatus::operationStateConflict,
                    reserved.operation,
                    createPayload);
            }
            return result(
                NativeTimerCreateOperationPreparationStatus::alreadyPrepared,
                reserved.operation,
                createPayload);
        case MutationOperationRepositoryStatus::idempotencyConflict:
            return result(
                NativeTimerCreateOperationPreparationStatus::idempotencyConflict,
                reserved.operation,
                createPayload);
        case MutationOperationRepositoryStatus::operationConflict:
            return result(
                NativeTimerCreateOperationPreparationStatus::operationConflict,
                reserved.operation,
                createPayload);
        case MutationOperationRepositoryStatus::invalid:
            return result(
                NativeTimerCreateOperationPreparationStatus::invalid,
                reserved.operation,
                createPayload);
        case MutationOperationRepositoryStatus::notFound:
        case MutationOperationRepositoryStatus::revisionConflict:
        case MutationOperationRepositoryStatus::stateConflict:
        case MutationOperationRepositoryStatus::storageError:
            return result(
                NativeTimerCreateOperationPreparationStatus::operationRepositoryError,
                reserved.operation,
                createPayload);
    }

    return result(
        NativeTimerCreateOperationPreparationStatus::operationRepositoryError,
        reserved.operation,
        createPayload);
}

}
