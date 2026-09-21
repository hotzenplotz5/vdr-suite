#include "NativeTimerModifyOperationPreparationService.h"

#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cstddef>

namespace vdrsuite::timers
{
namespace
{
constexpr std::size_t MaxIdentity = 160;
constexpr std::size_t MaxFingerprint = 4096;

bool identity(const std::string& value)
{
    return !value.empty() && value.size() <= MaxIdentity;
}

bool requestValid(const NativeTimerModifyOperationPreparationRequest& request)
{
    return identity(request.operationId)
        && identity(request.idempotencyKey)
        && identity(request.actorId)
        && !request.requestFingerprint.empty()
        && request.requestFingerprint.size() <= 512
        && identity(request.timerAssignmentId)
        && identity(request.expectedAssignmentRevision)
        && identity(request.expectedIntentRevision)
        && request.expectedAssignmentEpoch > 0
        && identity(request.nativeTimerBindingId)
        && identity(request.expectedBindingRevision)
        && identity(request.expectedBackendId)
        && request.expectedBackendGeneration > 0
        && identity(request.backendNativeTimerId)
        && !request.expectedCurrentFingerprint.empty()
        && request.expectedCurrentFingerprint.size() <= MaxFingerprint
        && nativeTimerSpecificationValid(request.expectedSpecification)
        && request.requestedAt > 0
        && (request.deadline == 0 ||
            request.deadline >= request.requestedAt);
}

NativeTimerSpecification specificationFor(
    const NativeTimerObservedState& state)
{
    NativeTimerSpecification specification;
    specification.channelId = state.channelId;
    specification.title = state.title;
    specification.directory = state.directory;
    specification.day = state.day;
    specification.weekdays = state.weekdays;
    specification.startTime = state.startTime;
    specification.endTime = state.endTime;
    specification.priority = state.priority;
    specification.lifetime = state.lifetime;
    specification.enabled = state.enabled;
    specification.vps = state.vps;
    return specification;
}

bool toggleShapeValid(
    const NativeTimerSpecification& current,
    const NativeTimerSpecification& expected)
{
    NativeTimerSpecification toggled = current;
    toggled.enabled = expected.enabled;
    return nativeTimerSpecificationFingerprint(toggled)
        == nativeTimerSpecificationFingerprint(expected)
        && current.enabled != expected.enabled;
}

NativeTimerModifyOperationPreparationResult result(
    NativeTimerModifyOperationPreparationStatus status,
    const vdrsuite::operations::MutationOperation& operation = {},
    const NativeTimerModifyOperationPayload& payload = {})
{
    NativeTimerModifyOperationPreparationResult value;
    value.status = status;
    value.operation = operation;
    value.payload = payload;
    return value;
}
}

NativeTimerModifyOperationPreparationService::
NativeTimerModifyOperationPreparationService(
    TimerIntentRepository& intentRepository,
    TimerAssignmentRepository& assignmentRepository,
    NativeTimerBindingRepository& bindingRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository),
      bindingRepository_(bindingRepository),
      operationRepository_(operationRepository)
{
}

NativeTimerModifyOperationPreparationResult
NativeTimerModifyOperationPreparationService::prepare(
    const NativeTimerModifyOperationPreparationRequest& request)
{
    using namespace vdrsuite::operations;
    if (!requestValid(request))
        return result(NativeTimerModifyOperationPreparationStatus::invalid);

    const auto assignmentResult =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (assignmentResult.status == TimerAssignmentRepositoryStatus::notFound)
        return result(
            NativeTimerModifyOperationPreparationStatus::assignmentNotFound);
    if (!assignmentResult.ok())
        return result(NativeTimerModifyOperationPreparationStatus::repositoryError);
    const TimerAssignment& assignment = assignmentResult.assignment;

    const auto intentResult =
        intentRepository_.findById(assignment.timerIntentId);
    if (intentResult.status == TimerIntentRepositoryStatus::notFound)
        return result(NativeTimerModifyOperationPreparationStatus::intentNotFound);
    if (!intentResult.ok())
        return result(NativeTimerModifyOperationPreparationStatus::repositoryError);
    const TimerIntent& intent = intentResult.intent;

    const auto bindingResult =
        bindingRepository_.findById(request.nativeTimerBindingId);
    if (bindingResult.status == NativeTimerBindingRepositoryStatus::notFound)
        return result(NativeTimerModifyOperationPreparationStatus::bindingNotFound);
    if (!bindingResult.ok())
        return result(NativeTimerModifyOperationPreparationStatus::repositoryError);
    const NativeTimerBinding& binding = bindingResult.binding;

    if (intent.intentRevision != request.expectedIntentRevision
        || assignment.intentRevision != request.expectedIntentRevision)
        return result(
            NativeTimerModifyOperationPreparationStatus::intentRevisionConflict);
    if (intent.state != TimerIntentState::active
        && !(request.kind == NativeTimerModifyKind::toggle
            && intent.state == TimerIntentState::paused))
        return result(
            NativeTimerModifyOperationPreparationStatus::intentStateConflict);
    if (assignment.assignmentRevision != request.expectedAssignmentRevision)
        return result(
            NativeTimerModifyOperationPreparationStatus::
                assignmentRevisionConflict);
    if (assignment.assignmentEpoch != request.expectedAssignmentEpoch)
        return result(
            NativeTimerModifyOperationPreparationStatus::assignmentEpochConflict);
    if (assignment.state != TimerAssignmentState::bound)
        return result(
            NativeTimerModifyOperationPreparationStatus::assignmentStateConflict);
    if (binding.bindingRevision != request.expectedBindingRevision)
        return result(
            NativeTimerModifyOperationPreparationStatus::bindingRevisionConflict);
    if (assignment.nativeTimerBindingId != binding.nativeTimerBindingId
        || binding.timerAssignmentId != assignment.timerAssignmentId
        || assignment.backendId != binding.backendId
        || request.backendNativeTimerId != binding.backendNativeTimerId)
        return result(
            NativeTimerModifyOperationPreparationStatus::identityConflict);
    if (binding.ownership != NativeTimerBindingOwnership::managed
        && binding.ownership != NativeTimerBindingOwnership::adopted)
        return result(
            NativeTimerModifyOperationPreparationStatus::ownershipConflict);
    if (assignment.backendId != request.expectedBackendId
        || binding.backendId != request.expectedBackendId)
        return result(
            NativeTimerModifyOperationPreparationStatus::identityConflict);
    if (assignment.backendGeneration != request.expectedBackendGeneration
        || binding.backendGeneration != request.expectedBackendGeneration)
        return result(
            NativeTimerModifyOperationPreparationStatus::generationConflict);
    if (binding.observedFingerprint != request.expectedCurrentFingerprint)
        return result(
            NativeTimerModifyOperationPreparationStatus::
                currentFingerprintConflict);
    if (binding.missingSince != 0
        || binding.driftState != NativeTimerBindingDriftState::none
        || binding.observedState.recording
        || binding.observedState.pending)
        return result(
            NativeTimerModifyOperationPreparationStatus::bindingStateConflict);

    if (request.kind == NativeTimerModifyKind::toggle
        && !toggleShapeValid(
            specificationFor(binding.observedState),
            request.expectedSpecification))
        return result(
            NativeTimerModifyOperationPreparationStatus::toggleShapeConflict);

    NativeTimerModifyOperationPayload payload;
    payload.kind = request.kind;
    payload.timerAssignmentId = assignment.timerAssignmentId;
    payload.expectedAssignmentRevision = assignment.assignmentRevision;
    payload.expectedIntentRevision = intent.intentRevision;
    payload.assignmentEpoch = assignment.assignmentEpoch;
    payload.nativeTimerBindingId = binding.nativeTimerBindingId;
    payload.expectedBindingRevision = binding.bindingRevision;
    payload.backendId = binding.backendId;
    payload.backendGeneration = binding.backendGeneration;
    payload.backendNativeTimerId = binding.backendNativeTimerId;
    payload.expectedCurrentFingerprint = binding.observedFingerprint;
    payload.expectedSpecification = request.expectedSpecification;
    const std::string serialized =
        serializeNativeTimerModifyOperationPayload(payload);
    const std::string fingerprint =
        nativeTimerModifyOperationPayloadFingerprint(payload);
    if (serialized.empty() || fingerprint.empty())
        return result(NativeTimerModifyOperationPreparationStatus::invalid);

    MutationOperation operation;
    operation.operationId = request.operationId;
    operation.idempotencyKey = request.idempotencyKey;
    operation.actorId = request.actorId;
    operation.backendId = binding.backendId;
    operation.backendGeneration = binding.backendGeneration;
    operation.resourceType = "NativeTimerBinding";
    operation.resourceId = binding.nativeTimerBindingId;
    operation.expectedRevision = binding.bindingRevision;
    operation.expectedResourceFingerprint = binding.observedFingerprint;
    operation.actionFamily = request.kind == NativeTimerModifyKind::update
        ? "timer.update" : "timer.toggle";
    operation.requestFingerprint = request.requestFingerprint;
    operation.requestedAt = request.requestedAt;
    operation.deadline = request.deadline;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = request.requestedAt;

    MutationOperationPayload durable;
    durable.operationId = request.operationId;
    durable.payloadType = "native.timer.modify";
    durable.payloadVersion = 1;
    durable.payload = serialized;
    durable.payloadFingerprint = fingerprint;

    const auto reserved =
        operationRepository_.reserveWithPayload(operation, durable);
    switch (reserved.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(
                NativeTimerModifyOperationPreparationStatus::prepared,
                reserved.operation,
                payload);
        case MutationOperationRepositoryStatus::idempotentReplay:
            return result(
                reserved.operation.state == MutationOperationState::accepted
                    ? NativeTimerModifyOperationPreparationStatus::alreadyPrepared
                    : NativeTimerModifyOperationPreparationStatus::
                        operationStateConflict,
                reserved.operation,
                payload);
        case MutationOperationRepositoryStatus::idempotencyConflict:
            return result(
                NativeTimerModifyOperationPreparationStatus::idempotencyConflict,
                reserved.operation,
                payload);
        case MutationOperationRepositoryStatus::operationConflict:
            return result(
                NativeTimerModifyOperationPreparationStatus::operationConflict,
                reserved.operation,
                payload);
        case MutationOperationRepositoryStatus::invalid:
            return result(
                NativeTimerModifyOperationPreparationStatus::invalid,
                reserved.operation,
                payload);
        default:
            return result(
                NativeTimerModifyOperationPreparationStatus::repositoryError,
                reserved.operation,
                payload);
    }
}

}
