#include "NativeTimerCreateAdmissionService.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "TimerIntentApplicationService.h"
#include "TimerIntentRepository.h"
#include "TimerAssignmentDesiredNativeTimerSpecification.h"
#include "EpgNativeTimerExecutionAuthority.h"
#include "EpgNativeTimerConflictGuard.h"
#include "EpgNativeTimerTimeGuard.h"

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
        && nativeTimerCreateAuxValid(request.aux)
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
    if (!request.aux.empty()) {
        const auto digest = vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(request.aux);
        if (digest.empty()) return {};
        appendField(fingerprint, digest);
    }
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
        && payload.aux == request.aux
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


std::string manualNativeTimerSubmissionDigest(
    const std::string& actorId, const std::string& backendId,
    const std::string& idempotencyKey)
{
    const auto field = [](const std::string& value) {
        return std::to_string(value.size()) + ":" + value + "|";
    };
    return vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
        "manual-native-timer-create/1|" + field(actorId) +
        field(backendId) + field(idempotencyKey));
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
    return admitImpl(request, false);
}

NativeTimerCreateAdmissionResult
NativeTimerCreateAdmissionService::admitInCurrentTransaction(
    const NativeTimerCreateAdmissionRequest& request)
{
    return admitImpl(request, true);
}

NativeTimerCreateAdmissionResult NativeTimerCreateAdmissionService::admitImpl(
    const NativeTimerCreateAdmissionRequest& request,
    bool currentTransaction)
{
    // Acquire the recursive lease even in nested mode. Its caller must
    // retain the outer lease and transaction until final COMMIT/ROLLBACK.
    auto lease = database_.acquireTransactionLease();
    if (currentTransaction != database_.transactionActive())
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
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

    if (!currentTransaction &&
        !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    const auto rollback = [this, currentTransaction]() {
        if (!currentTransaction) database_.execute("ROLLBACK;");
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
    preparation.aux = request.aux;
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

    if (!currentTransaction && !database_.execute("COMMIT;"))
    {
        rollback();
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    }

    return result(
        NativeTimerCreateAdmissionStatus::prepared,
        prepared.operation,
        prepared.payload);
}



NativeTimerCreateAdmissionResult
NativeTimerCreateAdmissionService::replayManualWindow(
    const ManualNativeTimerCreateReplayRequest& request,
    TimerIntentRepository& intents)
{
    using namespace vdrsuite::operations;
    if (!identity(request.actorId) || !identity(request.backendId) ||
        !identity(request.idempotencyKey) ||
        request.channelId.empty() || !nativeTimerCreateAuxValid(request.aux) ||
        request.startAt <= 0 || request.stopAt <= request.startAt)
        return result(NativeTimerCreateAdmissionStatus::invalid);

    const std::string digest = manualNativeTimerSubmissionDigest(
        request.actorId, request.backendId, request.idempotencyKey);
    if (digest.size() != 71U)
        return result(NativeTimerCreateAdmissionStatus::invalid);
    const std::string intentId = "manual-intent:" + digest.substr(7U);
    const std::string assignmentId = "manual-assignment:" + digest.substr(7U);

    auto lease = database_.acquireTransactionLease();
    const auto found = operationRepository_.findByIdempotencyScope(
        request.actorId, request.backendId, kResourceType, assignmentId,
        kActionFamily, request.idempotencyKey);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return result(NativeTimerCreateAdmissionStatus::assignmentNotFound);
    if (!found.ok())
        return result(NativeTimerCreateAdmissionStatus::repositoryError);

    const auto intent = intents.findById(intentId);
    const auto assignment = assignmentRepository_.findById(assignmentId);
    const auto payload = operationRepository_.findPayloadByOperationId(
        found.operation.operationId);
    NativeTimerCreateOperationPayload parsed;
    if (!intent.ok() || !assignment.ok() || !payload.ok() ||
        payload.payload.payloadType != kPayloadType ||
        payload.payload.payloadVersion != kPayloadVersion ||
        !parseNativeTimerCreateOperationPayload(
            payload.payload.payload, parsed) ||
        nativeTimerCreateOperationPayloadFingerprint(parsed) !=
            payload.payload.payloadFingerprint)
        return result(NativeTimerCreateAdmissionStatus::operationConflict);

    const auto& saved = intent.intent.spec;
    const auto& native = parsed.expectedSpecification;
    const bool matches =
        found.operation.actorId == request.actorId &&
        found.operation.backendId == request.backendId &&
        found.operation.resourceId == assignmentId &&
        found.operation.actionFamily == kActionFamily &&
        found.operation.idempotencyKey == request.idempotencyKey &&
        intent.intent.createdByActorId == request.actorId &&
        saved.intentType ==
            (request.weekdays == "-------"
                ? TimerIntentType::manualWindow
                : TimerIntentType::recurringSchedule) &&
        saved.ownerActorId == request.actorId &&
        saved.assignmentPolicy.preferredBackendIds ==
            std::vector<std::string>{request.backendId} &&
        saved.schedule.startAt == request.startAt &&
        saved.schedule.stopAt == request.stopAt &&
        saved.channelRequirement.sourceChannelId == request.channelId &&
        saved.recordingOptions.priority == request.priority &&
        saved.recordingOptions.lifetimeDays == request.lifetime &&
        saved.recordingOptions.vpsPreferred == request.vps &&
        assignment.assignment.timerIntentId == intentId &&
        assignment.assignment.backendId == request.backendId &&
        assignment.assignment.desiredNativeTimerSpecificationPresent &&
        timerAssignmentDesiredNativeTimerSpecificationEquivalent(
            assignment.assignment.desiredNativeTimerSpecification, native) &&
        native.channelId == request.channelId &&
        native.title == request.title &&
        native.directory == request.directory &&
        native.weekdays == request.weekdays &&
        native.priority == request.priority &&
        native.lifetime == request.lifetime &&
        native.enabled == request.enabled &&
        native.vps == request.vps &&
        parsed.aux == request.aux &&
        found.operation.expectedResourceFingerprint ==
            nativeTimerSpecificationFingerprint(native);
    if (!matches)
        return result(NativeTimerCreateAdmissionStatus::idempotencyConflict);
    return result(NativeTimerCreateAdmissionStatus::replayed,
        found.operation, parsed);
}

NativeTimerCreateAdmissionResult
NativeTimerCreateAdmissionService::admitManualWindow(
    const ManualNativeTimerCreateAdmissionRequest& request,
    TimerIntentRepository& intents,
    TimerIntentApplicationService& application)
{
    using namespace vdrsuite::operations;
    const auto& spec = request.draft.desiredNativeTimer;
    const auto& intentSpec = request.draft.intent;
    const auto& candidate = request.candidate;
    // The persisted recurrence rule must be derived from the exact VDR
    // weekday mask, not invented by a client or mixed with a one-shot draft.
    ManualTimerSourceEvidence source;
    source.backendId = request.backendId;
    source.sourceType = intentSpec.channelRequirement.sourceType;
    source.sourceId = intentSpec.channelRequirement.sourceId;
    source.backendChannelId =
        intentSpec.channelRequirement.sourceChannelId;
    source.timezone = intentSpec.schedule.timezone;
    source.startAt = intentSpec.schedule.startAt;
    source.stopAt = intentSpec.schedule.stopAt;
    const bool recurring = spec.weekdays != "-------";
    const auto projected = recurring
        ? makeRecurringTimerIntentDraft(spec, source, request.actorId)
        : makeManualWindowTimerIntentDraft(spec, source, request.actorId);
    if (!request.draft.valid ||
        !identity(request.actorId) || !identity(request.backendId) ||
        !identity(request.idempotencyKey) ||
        !nativeTimerSpecificationValid(spec) ||
        !nativeTimerCreateAuxValid(request.aux) ||
        !timerIntentValidSpec(intentSpec) ||
        !projected.valid ||
        timerIntentSemanticIdentity(projected.intent) !=
            timerIntentSemanticIdentity(intentSpec) ||
        intentSpec.ownerActorId != request.actorId ||
        intentSpec.assignmentPolicy.preferredBackendIds !=
            std::vector<std::string>{request.backendId} ||
        candidate.backendId != request.backendId ||
        !candidate.desiredNativeTimerSpecificationPresent ||
        !timerAssignmentDesiredNativeTimerSpecificationEquivalent(
            candidate.desiredNativeTimerSpecification, spec) ||
        candidate.channel.backendChannelId != spec.channelId ||
        request.requestedAt <= 0 ||
        request.requestedAt >
            std::numeric_limits<std::int64_t>::max() - 4 ||
        (request.deadline != 0 &&
         request.deadline < request.requestedAt + 3))
        return result(NativeTimerCreateAdmissionStatus::invalid);

    // Stable per actor/backend/key identities preserve replay after the
    // Assignment has entered provisioning and the Agent went away.
    const std::string digest = manualNativeTimerSubmissionDigest(
        request.actorId, request.backendId, request.idempotencyKey);
    if (digest.size() != 71U)
        return result(NativeTimerCreateAdmissionStatus::invalid);
    const std::string intentId = "manual-intent:" + digest.substr(7U);
    const std::string assignmentId = "manual-assignment:" + digest.substr(7U);

    auto lease = database_.acquireTransactionLease();
    if (database_.transactionActive() ||
        !intents.ensureSchema() ||
        !assignmentRepository_.ensureSchema() ||
        !operationRepository_.ensureSchema() ||
        !database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
        return result(NativeTimerCreateAdmissionStatus::repositoryError);
    const auto fail = [this](NativeTimerCreateAdmissionStatus status) {
        database_.execute("ROLLBACK;");
        return result(status);
    };
    const auto found = operationRepository_.findByIdempotencyScope(
        request.actorId, request.backendId, kResourceType,
        assignmentId, kActionFamily, request.idempotencyKey);
    if (found.ok())
    {
        const auto savedIntent = intents.findById(intentId);
        const auto savedAssignment = assignmentRepository_.findById(assignmentId);
        const auto payload = operationRepository_.findPayloadByOperationId(
            found.operation.operationId);
        NativeTimerCreateOperationPayload parsed;
        const bool same =
            savedIntent.ok() && savedAssignment.ok() && payload.ok() &&
            parseNativeTimerCreateOperationPayload(
                payload.payload.payload, parsed) &&
            savedIntent.intent.createdByActorId == request.actorId &&
            timerIntentSemanticIdentity(savedIntent.intent.spec) ==
                timerIntentSemanticIdentity(intentSpec) &&
            savedAssignment.assignment.backendId == request.backendId &&
            savedAssignment.assignment.desiredNativeTimerSpecificationPresent &&
            timerAssignmentDesiredNativeTimerSpecificationEquivalent(
                savedAssignment.assignment.desiredNativeTimerSpecification,
                spec) &&
            timerAssignmentDesiredNativeTimerSpecificationEquivalent(
                parsed.expectedSpecification, spec) &&
            parsed.aux == request.aux &&
            found.operation.actorId == request.actorId &&
            found.operation.backendId == request.backendId &&
            found.operation.resourceId == assignmentId &&
            found.operation.actionFamily == kActionFamily &&
            found.operation.idempotencyKey == request.idempotencyKey &&
            found.operation.expectedResourceFingerprint ==
                nativeTimerSpecificationFingerprint(spec);
        if (!same)
            return fail(NativeTimerCreateAdmissionStatus::idempotencyConflict);
        if (!database_.execute("COMMIT;"))
            return fail(NativeTimerCreateAdmissionStatus::repositoryError);
        return result(NativeTimerCreateAdmissionStatus::replayed,
            found.operation, parsed);
    }
    if (found.status != MutationOperationRepositoryStatus::notFound)
        return fail(NativeTimerCreateAdmissionStatus::repositoryError);

    // A partial previous submission must never launch a second timer.
    if (intents.findById(intentId).status != TimerIntentRepositoryStatus::notFound ||
        assignmentRepository_.findById(assignmentId).status !=
            TimerAssignmentRepositoryStatus::notFound)
        return fail(NativeTimerCreateAdmissionStatus::operationConflict);

    // Match established EPG one-shot deduplication before reserving a
    // second native operation for the same active semantic intent.
    const auto equivalents = intents.findEquivalent(intentSpec);
    if (!equivalents.ok())
        return fail(NativeTimerCreateAdmissionStatus::repositoryError);
    for (const auto& previous : equivalents.intents)
        if (!timerIntentTerminal(previous.state))
            return fail(NativeTimerCreateAdmissionStatus::operationConflict);

    // Do not let a manual submission bypass the existing native CREATE
    // clock / Agent lease / prospective conflict evidence required by
    // EPG-driven CREATE. The HTTP caller cannot provide these facts.
    const auto& clock = request.nativeClock;
    if (!vdrsuite::public_epg_timer::currentNativeTimerCreateExecutionAuthority(
            request.authoritativeAgent, request.nativeCreateProvider,
            candidate, request.requestedAt) ||
        !vdrsuite::public_epg_timer::prospectiveNativeTimerConflictClear(
            request.conflictProof, candidate, request.requestedAt) ||
        !clock.current ||
        clock.backendId != request.backendId ||
        clock.backendGeneration != candidate.currentBackendGeneration ||
        clock.executionAuthorityFence != candidate.executionAuthorityFence ||
        clock.providerInstanceEpoch !=
            request.nativeCreateProvider.providerInstanceEpoch ||
        clock.observedAt <= 0 ||
        clock.observedAt > request.requestedAt ||
        request.requestedAt - clock.observedAt > 15 ||
        clock.startUtcOffsetSeconds != clock.stopUtcOffsetSeconds)
        return fail(NativeTimerCreateAdmissionStatus::backendConflict);
    std::string nativeDay, nativeStart, nativeStopDay, nativeStop;
    if (!vdrsuite::public_epg_timer::projectEpgBoundaryToNative(
            intentSpec.schedule.startAt, clock.startUtcOffsetSeconds,
            false, nativeDay, nativeStart) ||
        !vdrsuite::public_epg_timer::projectEpgBoundaryToNative(
            intentSpec.schedule.stopAt, clock.stopUtcOffsetSeconds,
            true, nativeStopDay, nativeStop) ||
        nativeDay != spec.day || nativeStart != spec.startTime ||
        nativeStop != spec.endTime ||
        intentSpec.schedule.stopAt - intentSpec.schedule.startAt >=
            24 * 3600)
        return fail(NativeTimerCreateAdmissionStatus::backendConflict);

    // The production callback must recheck the live backend authority and
    // native mapping before ANY durable submission. The planner, not this
    // method, decides suitability of the candidate.
    if (!request.revalidateSourceAndExecutionAuthority)
        return fail(NativeTimerCreateAdmissionStatus::backendConflict);
    bool current = false;
    try {
        current = request.revalidateSourceAndExecutionAuthority();
    } catch (...) {
        return fail(NativeTimerCreateAdmissionStatus::backendConflict);
    }
    if (!current)
        return fail(NativeTimerCreateAdmissionStatus::backendConflict);

    TimerIntentApplicationRequest selectedRequest;
    selectedRequest.intent.timerIntentId = intentId;
    selectedRequest.intent.state = TimerIntentState::draft;
    selectedRequest.intent.createdByActorId = request.actorId;
    selectedRequest.intent.spec = intentSpec;
    selectedRequest.intent.createdAt = request.requestedAt;
    selectedRequest.intent.updatedAt = request.requestedAt;
    selectedRequest.intent.expiresAt = 0;
    selectedRequest.timerAssignmentId = assignmentId;
    selectedRequest.candidates = {candidate};
    selectedRequest.activatedAt = request.requestedAt + 1;
    selectedRequest.scheduledAt = request.requestedAt + 2;
    selectedRequest.provisioningAt = request.requestedAt + 3;
    const auto selected =
        application.submitAndSelectPrimaryInCurrentTransaction(selectedRequest);
    if (selected.status != TimerIntentApplicationStatus::assignmentSelected)
        return fail(selected.status == TimerIntentApplicationStatus::noEligibleBackend
            ? NativeTimerCreateAdmissionStatus::backendConflict
            : NativeTimerCreateAdmissionStatus::operationConflict);

    NativeTimerCreateAdmissionRequest native;
    native.actorId = request.actorId;
    native.idempotencyKey = request.idempotencyKey;
    native.timerAssignmentId = assignmentId;
    native.expectedAssignmentRevision =
        selected.assignment.assignmentRevision;
    native.expectedBackendId = request.backendId;
    native.requestedAt = request.requestedAt + 3;
    native.deadline = request.deadline;
    native.aux = request.aux;
    const auto prepared = admitInCurrentTransaction(native);
    if (prepared.status != NativeTimerCreateAdmissionStatus::prepared)
        return fail(prepared.status);
    if (!database_.execute("COMMIT;"))
        return fail(NativeTimerCreateAdmissionStatus::repositoryError);
    return prepared;
}

} // namespace vdrsuite::timers
