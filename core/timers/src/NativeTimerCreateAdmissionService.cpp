#include "NativeTimerCreateAdmissionService.h"

#include "Database.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingIdentity.h"
#include "NativeTimerCreateOperationPreparationService.h"
#include "NativeTimerSpecification.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 160;
constexpr std::size_t kMaxRequestFingerprintLength = 512;
constexpr const char* kResourceType = "TimerAssignment";
constexpr const char* kActionFamily = "timer.create";
constexpr const char* kPayloadType = "native.timer.create";
constexpr std::uint32_t kPayloadVersion = 1;

NativeTimerCreateAdmissionResult result(
    NativeTimerCreateAdmissionStatus status,
    const vdrsuite::operations::MutationOperation& operation = {},
    const NativeTimerCreateOperationPayload& payload = {})
{
    NativeTimerCreateAdmissionResult value;
    value.status = status;
    value.operation = operation;
    value.payload = payload;
    return value;
}

bool identity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool validRequest(const NativeTimerCreateAdmissionRequest& request)
{
    return identity(request.actorId)
        && identity(request.idempotencyKey)
        && identity(request.timerAssignmentId)
        && identity(request.expectedAssignmentRevision)
        && identity(request.expectedBackendId)
        && request.requestedAt > 0
        && (request.deadline == 0 || request.deadline >= request.requestedAt);
}

void appendField(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output += ':';
    output += value;
    output += '|';
}

std::string requestFingerprint(
    const NativeTimerCreateAdmissionRequest& request)
{
    if (!validRequest(request)) return {};

    std::string fingerprint = "native-timer-create-admission/1|";
    appendField(fingerprint, request.expectedBackendId);
    appendField(fingerprint, request.timerAssignmentId);
    appendField(fingerprint, request.expectedAssignmentRevision);
    return fingerprint.size() <= kMaxRequestFingerprintLength
        ? fingerprint
        : std::string();
}

bool operationMatchesReplay(
    const vdrsuite::operations::MutationOperation& operation,
    const NativeTimerCreateAdmissionRequest& request,
    const std::string& fingerprint)
{
    return operation.actorId == request.actorId
        && operation.backendId == request.expectedBackendId
        && operation.resourceType == kResourceType
        && operation.resourceId == request.timerAssignmentId
        && operation.actionFamily == kActionFamily
        && operation.idempotencyKey == request.idempotencyKey
        && operation.requestFingerprint == fingerprint
        && operation.verificationPolicy ==
            vdrsuite::operations::MutationOperationVerificationPolicy::
                readbackRequired;
}

bool payloadMatchesReplay(
    const NativeTimerCreateOperationPayload& payload,
    const vdrsuite::operations::MutationOperation& operation,
    const NativeTimerCreateAdmissionRequest& request)
{
    const std::string specificationFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    return payload.timerAssignmentId == request.timerAssignmentId
        && payload.backendId == request.expectedBackendId
        && payload.backendGeneration == operation.backendGeneration
        && payload.expectedAssignmentRevision == operation.expectedRevision
        && specificationFingerprint == operation.expectedResourceFingerprint
        && !payload.nativeTimerBindingId.empty()
        && specificationFingerprint.size() > 0;
}

NativeTimerCreateAdmissionStatus fulfillmentStatus(
    TimerAssignmentFulfillmentStatus status)
{
    switch (status)
    {
        case TimerAssignmentFulfillmentStatus::assignmentNotFound:
            return NativeTimerCreateAdmissionStatus::assignmentNotFound;
        case TimerAssignmentFulfillmentStatus::assignmentRevisionConflict:
            return NativeTimerCreateAdmissionStatus::assignmentRevisionConflict;
        case TimerAssignmentFulfillmentStatus::intentRevisionConflict:
            return NativeTimerCreateAdmissionStatus::intentRevisionConflict;
        case TimerAssignmentFulfillmentStatus::generationConflict:
            return NativeTimerCreateAdmissionStatus::generationConflict;
        case TimerAssignmentFulfillmentStatus::stateConflict:
        case TimerAssignmentFulfillmentStatus::alreadyProvisioning:
            return NativeTimerCreateAdmissionStatus::assignmentStateConflict;
        case TimerAssignmentFulfillmentStatus::invalid:
            return NativeTimerCreateAdmissionStatus::invalid;
        case TimerAssignmentFulfillmentStatus::repositoryConflict:
        case TimerAssignmentFulfillmentStatus::ownershipConflict:
            return NativeTimerCreateAdmissionStatus::assignmentStateConflict;
        case TimerAssignmentFulfillmentStatus::repositoryError:
            return NativeTimerCreateAdmissionStatus::repositoryError;
        case TimerAssignmentFulfillmentStatus::provisioningStarted:
            return NativeTimerCreateAdmissionStatus::prepared;
        case TimerAssignmentFulfillmentStatus::bound:
        case TimerAssignmentFulfillmentStatus::alreadyBound:
        case TimerAssignmentFulfillmentStatus::bindingNotFound:
        case TimerAssignmentFulfillmentStatus::bindingRevisionConflict:
        case TimerAssignmentFulfillmentStatus::identityConflict:
        case TimerAssignmentFulfillmentStatus::bindingStateConflict:
            return NativeTimerCreateAdmissionStatus::assignmentStateConflict;
    }
    return NativeTimerCreateAdmissionStatus::repositoryError;
}

NativeTimerCreateAdmissionStatus preparationStatus(
    NativeTimerCreateOperationPreparationStatus status)
{
    switch (status)
    {
        case NativeTimerCreateOperationPreparationStatus::prepared:
            return NativeTimerCreateAdmissionStatus::prepared;
        case NativeTimerCreateOperationPreparationStatus::alreadyPrepared:
            return NativeTimerCreateAdmissionStatus::replayed;
        case NativeTimerCreateOperationPreparationStatus::assignmentNotFound:
            return NativeTimerCreateAdmissionStatus::assignmentNotFound;
        case NativeTimerCreateOperationPreparationStatus::assignmentRevisionConflict:
            return NativeTimerCreateAdmissionStatus::assignmentRevisionConflict;
        case NativeTimerCreateOperationPreparationStatus::assignmentStateConflict:
        case NativeTimerCreateOperationPreparationStatus::intentStateConflict:
        case NativeTimerCreateOperationPreparationStatus::assignmentEpochConflict:
        case NativeTimerCreateOperationPreparationStatus::bindingAlreadyPresent:
            return NativeTimerCreateAdmissionStatus::assignmentStateConflict;
        case NativeTimerCreateOperationPreparationStatus::backendConflict:
        case NativeTimerCreateOperationPreparationStatus::channelConflict:
            return NativeTimerCreateAdmissionStatus::backendConflict;
        case NativeTimerCreateOperationPreparationStatus::generationConflict:
            return NativeTimerCreateAdmissionStatus::generationConflict;
        case NativeTimerCreateOperationPreparationStatus::intentRevisionConflict:
            return NativeTimerCreateAdmissionStatus::intentRevisionConflict;
        case NativeTimerCreateOperationPreparationStatus::idempotencyConflict:
            return NativeTimerCreateAdmissionStatus::idempotencyConflict;
        case NativeTimerCreateOperationPreparationStatus::operationConflict:
        case NativeTimerCreateOperationPreparationStatus::operationStateConflict:
            return NativeTimerCreateAdmissionStatus::operationConflict;
        case NativeTimerCreateOperationPreparationStatus::intentNotFound:
        case NativeTimerCreateOperationPreparationStatus::operationRepositoryError:
            return NativeTimerCreateAdmissionStatus::repositoryError;
        case NativeTimerCreateOperationPreparationStatus::invalid:
            return NativeTimerCreateAdmissionStatus::invalid;
    }
    return NativeTimerCreateAdmissionStatus::repositoryError;
}

std::int64_t provisioningTimestamp(
    std::int64_t requestedAt,
    std::int64_t assignmentUpdatedAt)
{
    if (requestedAt <= 0 ||
        assignmentUpdatedAt == std::numeric_limits<std::int64_t>::max())
    {
        return 0;
    }
    return std::max(requestedAt, assignmentUpdatedAt + 1);
}
} // namespace

NativeTimerCreateAdmissionService::NativeTimerCreateAdmissionService(
    Database& database,
    TimerAssignmentRepository& assignmentRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    TimerAssignmentFulfillmentService& fulfillmentService,
    NativeTimerCreateOperationPreparationService& preparationService)
    : database_(database),
      assignmentRepository_(assignmentRepository),
      operationRepository_(operationRepository),
      fulfillmentService_(fulfillmentService),
      preparationService_(preparationService)
{
}

NativeTimerCreateAdmissionResult
NativeTimerCreateAdmissionService::replayIfPresent(
    const NativeTimerCreateAdmissionRequest& request,
    const std::string& fingerprint,
    bool& present)
{
    using namespace vdrsuite::operations;

    present = false;
    const auto found = operationRepository_.findByIdempotencyScope(
        request.actorId,
        request.expectedBackendId,
        kResourceType,
        request.timerAssignmentId,
        kActionFamily,
        request.idempotencyKey);

    if (found.status == MutationOperationRepositoryStatus::notFound)
        return result(NativeTimerCreateAdmissionStatus::prepared);
    if (found.status == MutationOperationRepositoryStatus::invalid)
        return result(NativeTimerCreateAdmissionStatus::invalid);
    if (!found.ok())
        return result(NativeTimerCreateAdmissionStatus::repositoryError);

    present = true;
    if (!operationMatchesReplay(found.operation, request, fingerprint))
    {
        return result(
            NativeTimerCreateAdmissionStatus::idempotencyConflict,
            found.operation);
    }

    const auto payloadResult =
        operationRepository_.findPayloadByOperationId(
            found.operation.operationId);
    if (!payloadResult.ok() ||
        payloadResult.payload.payloadType != kPayloadType ||
        payloadResult.payload.payloadVersion != kPayloadVersion)
    {
        return result(
            payloadResult.status == MutationOperationRepositoryStatus::storageError
                ? NativeTimerCreateAdmissionStatus::repositoryError
                : NativeTimerCreateAdmissionStatus::operationConflict,
            found.operation);
    }

    NativeTimerCreateOperationPayload payload;
    if (!parseNativeTimerCreateOperationPayload(
            payloadResult.payload.payload,
            payload) ||
        nativeTimerCreateOperationPayloadFingerprint(payload) !=
            payloadResult.payload.payloadFingerprint ||
        !payloadMatchesReplay(
            payload,
            found.operation,
            request))
    {
        return result(
            NativeTimerCreateAdmissionStatus::operationConflict,
            found.operation);
    }

    return result(
        NativeTimerCreateAdmissionStatus::replayed,
        found.operation,
        payload);
}

NativeTimerCreateAdmissionResult NativeTimerCreateAdmissionService::admit(
    const NativeTimerCreateAdmissionRequest& request)
{
    if (!validRequest(request))
        return result(NativeTimerCreateAdmissionStatus::invalid);

    const std::string fingerprint = requestFingerprint(request);
    if (fingerprint.empty())
        return result(NativeTimerCreateAdmissionStatus::invalid);

    bool replayPresent = false;
    NativeTimerCreateAdmissionResult replay =
        replayIfPresent(request, fingerprint, replayPresent);
    if (replayPresent ||
        replay.status != NativeTimerCreateAdmissionStatus::prepared)
    {
        return replay;
    }

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    const auto rollback = [this]() {
        database_.execute("ROLLBACK;");
    };

    replay = replayIfPresent(request, fingerprint, replayPresent);
    if (replayPresent ||
        replay.status != NativeTimerCreateAdmissionStatus::prepared)
    {
        rollback();
        return replay;
    }

    const auto found =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (found.status == TimerAssignmentRepositoryStatus::notFound)
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::assignmentNotFound);
    }
    if (!found.ok())
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    }

    const TimerAssignment& selected = found.assignment;
    if (selected.backendId != request.expectedBackendId)
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::assignmentNotFound);
    }
    if (selected.assignmentRevision != request.expectedAssignmentRevision)
    {
        rollback();
        return result(
            NativeTimerCreateAdmissionStatus::assignmentRevisionConflict);
    }
    if (selected.state != TimerAssignmentState::selected ||
        !selected.nativeTimerBindingId.empty())
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::assignmentStateConflict);
    }
    if (!selected.desiredNativeTimerSpecificationPresent ||
        !nativeTimerSpecificationValid(
            selected.desiredNativeTimerSpecification) ||
        selected.desiredNativeTimerSpecification.channelId !=
            selected.channelBinding.backendChannelId)
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::specificationUnavailable);
    }

    const std::int64_t provisioningAt =
        provisioningTimestamp(
            request.requestedAt,
            selected.updatedAt);
    if (provisioningAt <= selected.updatedAt)
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::invalid);
    }

    const std::string operationId =
        vdrsuite::operations::generateMutationOperationId();
    const std::string bindingId =
        generateNativeTimerBindingId();
    if (!vdrsuite::operations::mutationOperationIdCanonical(operationId) ||
        !nativeTimerBindingIdCanonical(bindingId))
    {
        rollback();
        return result(
            NativeTimerCreateAdmissionStatus::identityGenerationFailed);
    }

    const auto provisioned =
        fulfillmentService_.beginProvisioningInCurrentTransaction(
            selected.timerAssignmentId,
            selected.assignmentRevision,
            selected.intentRevision,
            selected.backendGeneration,
            provisioningAt);
    if (provisioned.status !=
        TimerAssignmentFulfillmentStatus::provisioningStarted)
    {
        const NativeTimerCreateAdmissionStatus status =
            fulfillmentStatus(provisioned.status);
        rollback();
        return result(status);
    }

    NativeTimerCreateOperationPreparationRequest preparation;
    preparation.operationId = operationId;
    preparation.idempotencyKey = request.idempotencyKey;
    preparation.actorId = request.actorId;
    preparation.requestFingerprint = fingerprint;
    preparation.timerAssignmentId =
        provisioned.assignment.timerAssignmentId;
    preparation.expectedAssignmentRevision =
        provisioned.assignment.assignmentRevision;
    preparation.expectedIntentRevision =
        provisioned.assignment.intentRevision;
    preparation.expectedAssignmentEpoch =
        provisioned.assignment.assignmentEpoch;
    preparation.nativeTimerBindingId = bindingId;
    preparation.expectedBackendId =
        provisioned.assignment.backendId;
    preparation.expectedBackendGeneration =
        provisioned.assignment.backendGeneration;
    preparation.expectedSpecification =
        provisioned.assignment.desiredNativeTimerSpecification;
    preparation.requestedAt = request.requestedAt;
    preparation.deadline = request.deadline;

    const auto prepared =
        preparationService_.prepareInCurrentTransaction(
            preparation);
    if (prepared.status !=
        NativeTimerCreateOperationPreparationStatus::prepared)
    {
        const NativeTimerCreateAdmissionStatus status =
            preparationStatus(prepared.status);
        rollback();
        return result(status);
    }

    if (!database_.execute("COMMIT;"))
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    }

    return result(
        NativeTimerCreateAdmissionStatus::prepared,
        prepared.operation,
        prepared.payload);
}

} // namespace vdrsuite::timers
