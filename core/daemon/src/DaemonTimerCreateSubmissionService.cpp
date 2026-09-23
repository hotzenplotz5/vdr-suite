#include "DaemonTimerCreateSubmissionService.h"

#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateActivation.h"
#include "BackendAgentNativeTimerCreateReservation.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateOperationPreparationService.h"
#include "SecurityIdentity.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <chrono>
#include <cstdint>
#include <openssl/evp.h>
#include <string>

namespace
{
constexpr std::int64_t SubmissionDeadlineSeconds = 300;

DaemonTimerCreateSubmissionResult result(
    DaemonTimerCreateSubmissionStatus status,
    const vdrsuite::operations::MutationOperation& operation = {})
{
    DaemonTimerCreateSubmissionResult value;
    value.status = status;
    value.operation = operation;
    return value;
}

DaemonTimerCreateReplayResult replayResult(
    DaemonTimerCreateReplayStatus status,
    const std::string& expectedRevision = {})
{
    DaemonTimerCreateReplayResult value;
    value.status = status;
    value.expectedAssignmentRevision = expectedRevision;
    return value;
}

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string sha256Token(const std::string& value)
{
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (!context) return {};

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned length = 0;
    const bool ok =
        EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
        EVP_DigestUpdate(context, value.data(), value.size()) == 1 &&
        EVP_DigestFinal_ex(context, digest, &length) == 1;
    EVP_MD_CTX_free(context);
    if (!ok || length != 32U) return {};

    static constexpr char Hex[] = "0123456789abcdef";
    std::string token = "sha256:";
    token.reserve(71U);
    for (unsigned index = 0; index < length; ++index)
    {
        token.push_back(Hex[(digest[index] >> 4U) & 0x0fU]);
        token.push_back(Hex[digest[index] & 0x0fU]);
    }
    return token;
}

void appendCanonical(std::string& output, const std::string& value)
{
    output += std::to_string(value.size());
    output.push_back(':');
    output += value;
    output.push_back('|');
}

std::string submissionFingerprint(
    const std::string& expectedRevision,
    const vdrsuite::timers::NativeTimerSpecification& specification)
{
    const std::string specificationFingerprint =
        vdrsuite::timers::nativeTimerSpecificationFingerprint(specification);
    if (specificationFingerprint.empty()) return {};

    std::string canonical = "public-timer-create/1|";
    appendCanonical(canonical, expectedRevision);
    appendCanonical(canonical, specificationFingerprint);
    return sha256Token(canonical);
}

vdrsuite::agent::BackendAgentNativeTimerCreateSpecification agentSpecification(
    const vdrsuite::timers::NativeTimerSpecification& specification)
{
    vdrsuite::agent::BackendAgentNativeTimerCreateSpecification result;
    result.channelId = specification.channelId;
    result.title = specification.title;
    result.directory = specification.directory;
    result.day = specification.day;
    result.weekdays = specification.weekdays;
    result.startTime = specification.startTime;
    result.endTime = specification.endTime;
    result.priority = specification.priority;
    result.lifetime = specification.lifetime;
    result.enabled = specification.enabled;
    result.vps = specification.vps;
    return result;
}

RequestSecurityContext systemContext(
    const DaemonTimerCreateSubmissionRequest& request)
{
    RequestSecurityContext context;
    context.requestId = request.requestId;
    context.correlationId = request.correlationId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor.actorId = "system:public-timer-create";
    context.actor.type = ActorType::System;
    context.actor.displayName = "Public Timer CREATE dispatcher";
    context.actor.active = true;
    return context;
}

DaemonTimerCreateSubmissionStatus preparationFailure(
    vdrsuite::timers::NativeTimerCreateOperationPreparationStatus status)
{
    using Status =
        vdrsuite::timers::NativeTimerCreateOperationPreparationStatus;
    switch (status)
    {
        case Status::intentNotFound:
        case Status::assignmentNotFound:
            return DaemonTimerCreateSubmissionStatus::notFound;
        case Status::intentRevisionConflict:
        case Status::assignmentRevisionConflict:
            return DaemonTimerCreateSubmissionStatus::revisionConflict;
        case Status::generationConflict:
            return DaemonTimerCreateSubmissionStatus::generationConflict;
        case Status::idempotencyConflict:
        case Status::operationConflict:
            return DaemonTimerCreateSubmissionStatus::idempotencyConflict;
        case Status::intentStateConflict:
        case Status::assignmentEpochConflict:
        case Status::assignmentStateConflict:
        case Status::backendConflict:
        case Status::channelConflict:
        case Status::bindingAlreadyPresent:
        case Status::operationStateConflict:
            return DaemonTimerCreateSubmissionStatus::stateConflict;
        case Status::invalid:
            return DaemonTimerCreateSubmissionStatus::invalid;
        case Status::operationRepositoryError:
        case Status::prepared:
        case Status::alreadyPrepared:
            return DaemonTimerCreateSubmissionStatus::unavailable;
    }
    return DaemonTimerCreateSubmissionStatus::unavailable;
}

DaemonTimerCreateSubmissionStatus reservationFailure(
    const std::string& reason)
{
    if (reason == "native_timer_create_backend_generation_conflict")
        return DaemonTimerCreateSubmissionStatus::generationConflict;
    if (reason == "active_agent_lease_required")
        return DaemonTimerCreateSubmissionStatus::backendUnavailable;
    if (reason == "native_timer_create_provider_selection_missing" ||
        reason == "native_timer_create_provider_selection_stale" ||
        reason == "local_provider_ownership_required" ||
        reason == "local_provider_capability_not_observed" ||
        reason == "command_capability_required")
        return DaemonTimerCreateSubmissionStatus::capabilityUnavailable;
    if (reason == "native_timer_create_active_assignment_conflict" ||
        reason == "native_timer_create_reservation_conflict")
        return DaemonTimerCreateSubmissionStatus::stateConflict;
    if (reason == "invalid_native_timer_create_reservation_request")
        return DaemonTimerCreateSubmissionStatus::invalid;
    return DaemonTimerCreateSubmissionStatus::unavailable;
}
}

DaemonTimerCreateSubmissionService::DaemonTimerCreateSubmissionService(
    vdrsuite::timers::TimerIntentRepository& intentRepository,
    vdrsuite::timers::TimerAssignmentRepository& assignmentRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerCreateOperationPreparationService&
        preparationService,
    BackendAgentRepository& agentRepository,
    vdrsuite::agent::BackendAgentNativeTimerCreateReservationService&
        reservationService,
    vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService,
    vdrsuite::agent::BackendAgentNativeTimerCreateActivationService&
        activationService)
    : intentRepository_(intentRepository),
      assignmentRepository_(assignmentRepository),
      operationRepository_(operationRepository),
      preparationService_(preparationService),
      agentRepository_(agentRepository),
      reservationService_(reservationService),
      dispatchService_(dispatchService),
      activationService_(activationService)
{
}

DaemonTimerCreateReplayResult
DaemonTimerCreateSubmissionService::lookupReplay(
    const DaemonTimerCreateReplayRequest& request)
{
    using namespace vdrsuite::operations;
    using namespace vdrsuite::timers;

    if (request.timerAssignmentId.empty() ||
        request.backendId.empty() ||
        request.actorId.empty() ||
        request.idempotencyKey.empty() ||
        !request.requestedSpecification.channelId.empty())
    {
        return replayResult(DaemonTimerCreateReplayStatus::invalid);
    }

    const auto existing =
        operationRepository_.findByIdempotencyScope(
            request.actorId,
            request.backendId,
            "TimerAssignment",
            request.timerAssignmentId,
            "timer.create",
            request.idempotencyKey);
    if (existing.status ==
        MutationOperationRepositoryStatus::notFound)
    {
        return replayResult(DaemonTimerCreateReplayStatus::notFound);
    }
    if (existing.status ==
        MutationOperationRepositoryStatus::invalid)
    {
        return replayResult(DaemonTimerCreateReplayStatus::invalid);
    }
    if (!existing.ok())
        return replayResult(DaemonTimerCreateReplayStatus::unavailable);

    const MutationOperation& operation = existing.operation;
    if (operation.expectedRevision.empty() ||
        operation.requestFingerprint.empty() ||
        operation.actorId != request.actorId ||
        operation.backendId != request.backendId ||
        operation.resourceType != "TimerAssignment" ||
        operation.resourceId != request.timerAssignmentId ||
        operation.actionFamily != "timer.create")
    {
        return replayResult(
            DaemonTimerCreateReplayStatus::idempotencyConflict);
    }

    const auto storedPayload =
        operationRepository_.findPayloadByOperationId(
            operation.operationId);
    NativeTimerCreateOperationPayload payload;
    if (!storedPayload.ok() ||
        storedPayload.payload.payloadType != "native.timer.create" ||
        storedPayload.payload.payloadVersion != 1U ||
        !parseNativeTimerCreateOperationPayload(
            storedPayload.payload.payload,
            payload) ||
        payload.timerAssignmentId != request.timerAssignmentId ||
        payload.expectedAssignmentRevision !=
            operation.expectedRevision ||
        payload.backendId != request.backendId)
    {
        return replayResult(
            DaemonTimerCreateReplayStatus::idempotencyConflict);
    }

    NativeTimerSpecification replaySpecification =
        request.requestedSpecification;
    replaySpecification.channelId =
        payload.expectedSpecification.channelId;
    const std::string fingerprint =
        submissionFingerprint(
            operation.expectedRevision,
            replaySpecification);
    if (fingerprint.empty())
        return replayResult(DaemonTimerCreateReplayStatus::unavailable);

    if (fingerprint != operation.requestFingerprint ||
        nativeTimerSpecificationFingerprint(
            replaySpecification) !=
        nativeTimerSpecificationFingerprint(
            payload.expectedSpecification))
    {
        return replayResult(
            DaemonTimerCreateReplayStatus::idempotencyConflict);
    }

    return replayResult(
        DaemonTimerCreateReplayStatus::matched,
        operation.expectedRevision);
}

DaemonTimerCreateSubmissionResult
DaemonTimerCreateSubmissionService::submit(
    const DaemonTimerCreateSubmissionRequest& request)
{
    using namespace vdrsuite::operations;
    using namespace vdrsuite::timers;

    if (request.timerAssignmentId.empty() ||
        request.backendId.empty() ||
        request.actorId.empty() ||
        request.idempotencyKey.empty() ||
        request.expectedAssignmentRevision.empty() ||
        !request.requestedSpecification.channelId.empty())
    {
        return result(DaemonTimerCreateSubmissionStatus::invalid);
    }

    const std::int64_t now = nowSeconds();
    const std::int64_t deadline = now + SubmissionDeadlineSeconds;

    const auto assignmentResult =
        assignmentRepository_.findById(request.timerAssignmentId);
    if (assignmentResult.status ==
        TimerAssignmentRepositoryStatus::notFound)
    {
        return result(DaemonTimerCreateSubmissionStatus::notFound);
    }
    if (!assignmentResult.ok())
        return result(DaemonTimerCreateSubmissionStatus::unavailable);

    const TimerAssignment& assignment = assignmentResult.assignment;
    if (assignment.backendId != request.backendId)
        return result(DaemonTimerCreateSubmissionStatus::notFound);
    if (assignment.assignmentRevision !=
        request.expectedAssignmentRevision)
    {
        return result(DaemonTimerCreateSubmissionStatus::revisionConflict);
    }
    if (assignment.state != TimerAssignmentState::provisioning ||
        !assignment.nativeTimerBindingId.empty())
    {
        return result(DaemonTimerCreateSubmissionStatus::stateConflict);
    }

    const auto intentResult =
        intentRepository_.findById(assignment.timerIntentId);
    if (intentResult.status == TimerIntentRepositoryStatus::notFound)
        return result(DaemonTimerCreateSubmissionStatus::notFound);
    if (!intentResult.ok())
        return result(DaemonTimerCreateSubmissionStatus::unavailable);
    if (intentResult.intent.intentRevision != assignment.intentRevision)
        return result(DaemonTimerCreateSubmissionStatus::revisionConflict);

    const auto agent = agentRepository_.findAgentForBackend(
        request.backendId);
    if (!agent.has_value() || agent->revoked || agent->incompatible ||
        agent->agentInstanceId.empty() ||
        agent->backendGeneration == 0 ||
        agent->leaseExpiresAt < now)
    {
        return result(DaemonTimerCreateSubmissionStatus::backendUnavailable);
    }
    if (agent->backendGeneration != assignment.backendGeneration)
        return result(DaemonTimerCreateSubmissionStatus::generationConflict);

    NativeTimerSpecification specification =
        request.requestedSpecification;
    specification.channelId =
        assignment.channelBinding.backendChannelId;
    if (!nativeTimerSpecificationValid(specification))
        return result(DaemonTimerCreateSubmissionStatus::invalid);

    const std::string requestFingerprint =
        submissionFingerprint(
            request.expectedAssignmentRevision,
            specification);
    if (requestFingerprint.empty())
        return result(DaemonTimerCreateSubmissionStatus::unavailable);

    const auto existing =
        operationRepository_.findByIdempotencyScope(
            request.actorId,
            request.backendId,
            "TimerAssignment",
            request.timerAssignmentId,
            "timer.create",
            request.idempotencyKey);

    NativeTimerCreateOperationPreparationResult prepared;
    std::int64_t effectiveDeadline = deadline;

    if (existing.ok())
    {
        const MutationOperation& operation = existing.operation;
        if (operation.requestFingerprint != requestFingerprint ||
            operation.expectedRevision !=
                request.expectedAssignmentRevision ||
            operation.actorId != request.actorId ||
            operation.backendId != request.backendId ||
            operation.resourceType != "TimerAssignment" ||
            operation.resourceId != request.timerAssignmentId ||
            operation.actionFamily != "timer.create")
        {
            return result(
                DaemonTimerCreateSubmissionStatus::idempotencyConflict,
                operation);
        }

        if (operation.state == MutationOperationState::dispatching)
        {
            const auto activated =
                activationService_.activateDispatching(
                    operation.operationId);
            if (!activated.ok())
            {
                if (activated.status ==
                    vdrsuite::agent::
                        BackendAgentNativeTimerCreateActivationStatus::
                            providerSelectionStale)
                {
                    return result(
                        DaemonTimerCreateSubmissionStatus::
                            capabilityUnavailable,
                        operation);
                }
                return result(
                    DaemonTimerCreateSubmissionStatus::unavailable,
                    operation);
            }
            const auto refreshed =
                operationRepository_.findById(
                    operation.operationId);
            return refreshed.ok()
                ? result(
                    DaemonTimerCreateSubmissionStatus::accepted,
                    refreshed.operation)
                : result(
                    DaemonTimerCreateSubmissionStatus::accepted,
                    operation);
        }

        if (operation.state != MutationOperationState::accepted)
        {
            return result(
                DaemonTimerCreateSubmissionStatus::accepted,
                operation);
        }

        const auto storedPayload =
            operationRepository_.findPayloadByOperationId(
                operation.operationId);
        NativeTimerCreateOperationPayload payload;
        if (!storedPayload.ok() ||
            storedPayload.payload.payloadType != "native.timer.create" ||
            storedPayload.payload.payloadVersion != 1U ||
            !parseNativeTimerCreateOperationPayload(
                storedPayload.payload.payload,
                payload) ||
            payload.timerAssignmentId != request.timerAssignmentId ||
            payload.expectedAssignmentRevision !=
                request.expectedAssignmentRevision ||
            payload.expectedIntentRevision != assignment.intentRevision ||
            payload.assignmentEpoch != assignment.assignmentEpoch ||
            payload.backendId != request.backendId ||
            payload.backendGeneration != assignment.backendGeneration ||
            nativeTimerSpecificationFingerprint(
                payload.expectedSpecification) !=
                nativeTimerSpecificationFingerprint(specification))
        {
            return result(
                DaemonTimerCreateSubmissionStatus::idempotencyConflict,
                operation);
        }
        if (operation.deadline > 0 && operation.deadline < now)
        {
            return result(
                DaemonTimerCreateSubmissionStatus::stateConflict,
                operation);
        }

        prepared.status =
            NativeTimerCreateOperationPreparationStatus::alreadyPrepared;
        prepared.operation = operation;
        prepared.payload = payload;
        effectiveDeadline = operation.deadline;
    }
    else if (existing.status ==
        MutationOperationRepositoryStatus::notFound)
    {
        const std::string operationId =
            backendAgentGenerateOpaqueId("op_", 12);
        const std::string nativeTimerBindingId =
            backendAgentGenerateOpaqueId("ntb_", 12);
        if (operationId.empty() || nativeTimerBindingId.empty())
            return result(DaemonTimerCreateSubmissionStatus::unavailable);

        NativeTimerCreateOperationPreparationRequest preparation;
        preparation.operationId = operationId;
        preparation.idempotencyKey = request.idempotencyKey;
        preparation.actorId = request.actorId;
        preparation.requestFingerprint = requestFingerprint;
        preparation.timerAssignmentId = request.timerAssignmentId;
        preparation.expectedAssignmentRevision =
            request.expectedAssignmentRevision;
        preparation.expectedIntentRevision =
            assignment.intentRevision;
        preparation.expectedAssignmentEpoch =
            assignment.assignmentEpoch;
        preparation.nativeTimerBindingId = nativeTimerBindingId;
        preparation.expectedBackendId = request.backendId;
        preparation.expectedBackendGeneration =
            assignment.backendGeneration;
        preparation.expectedSpecification = specification;
        preparation.requestedAt = now;
        preparation.deadline = deadline;

        prepared = preparationService_.prepare(preparation);
        if (!prepared.ok())
        {
            return result(
                preparationFailure(prepared.status),
                prepared.operation);
        }
    }
    else if (existing.status ==
        MutationOperationRepositoryStatus::invalid)
    {
        return result(DaemonTimerCreateSubmissionStatus::invalid);
    }
    else
    {
        return result(DaemonTimerCreateSubmissionStatus::unavailable);
    }

    vdrsuite::agent::BackendAgentNativeTimerCreateReservationRequest
        reservation;
    reservation.operationId =
        prepared.operation.operationId;
    reservation.operationRevision =
        prepared.operation.operationRevision;
    reservation.timerAssignmentId =
        prepared.payload.timerAssignmentId;
    reservation.expectedAssignmentRevision =
        prepared.payload.expectedAssignmentRevision;
    reservation.expectedIntentRevision =
        prepared.payload.expectedIntentRevision;
    reservation.assignmentEpoch =
        prepared.payload.assignmentEpoch;
    reservation.nativeTimerBindingId =
        prepared.payload.nativeTimerBindingId;
    reservation.backendId =
        prepared.payload.backendId;
    reservation.backendGeneration =
        prepared.payload.backendGeneration;
    reservation.expectedSpecification =
        agentSpecification(
            prepared.payload.expectedSpecification);
    reservation.expectedSpecificationFingerprint =
        prepared.operation.expectedResourceFingerprint;

    const auto reserved =
        reservationService_.reserve(
            systemContext(request),
            reservation,
            now,
            effectiveDeadline);
    if (!reserved.accepted)
    {
        return result(
            reservationFailure(reserved.reasonCode),
            prepared.operation);
    }

    NativeTimerCreateDispatchClaimRequest claim;
    claim.operationId = prepared.operation.operationId;
    claim.expectedOperationRevision =
        prepared.operation.operationRevision;
    claim.timerAssignmentId =
        prepared.payload.timerAssignmentId;
    claim.nativeTimerBindingId =
        prepared.payload.nativeTimerBindingId;
    claim.backendId = prepared.payload.backendId;
    claim.backendGeneration =
        prepared.payload.backendGeneration;
    claim.expectedSpecificationFingerprint =
        prepared.operation.expectedResourceFingerprint;
    claim.reservation.commandId =
        reserved.assignment.commandId;
    claim.reservation.requestFingerprint =
        reserved.assignment.requestFingerprint;

    const auto claimed =
        dispatchService_.claimAfterReservation(
            claim,
            now);
    if (!claimed.ok())
    {
        const auto current =
            operationRepository_.findById(
                prepared.operation.operationId);
        if (current.ok() &&
            current.operation.state ==
                MutationOperationState::dispatching)
        {
            const auto activated =
                activationService_.activateDispatching(
                    current.operation.operationId);
            if (!activated.ok())
                return result(
                    DaemonTimerCreateSubmissionStatus::unavailable,
                    current.operation);
            return result(
                DaemonTimerCreateSubmissionStatus::accepted,
                current.operation);
        }

        if (claimed.status ==
            NativeTimerCreateDispatchClaimStatus::
                operationRevisionConflict)
        {
            return result(
                DaemonTimerCreateSubmissionStatus::revisionConflict,
                prepared.operation);
        }
        if (claimed.status ==
                NativeTimerCreateDispatchClaimStatus::
                    operationStateConflict ||
            claimed.status ==
                NativeTimerCreateDispatchClaimStatus::
                    identityConflict ||
            claimed.status ==
                NativeTimerCreateDispatchClaimStatus::
                    payloadConflict ||
            claimed.status ==
                NativeTimerCreateDispatchClaimStatus::
                    deadlineExpired)
        {
            return result(
                DaemonTimerCreateSubmissionStatus::stateConflict,
                prepared.operation);
        }
        return result(
            DaemonTimerCreateSubmissionStatus::unavailable,
            prepared.operation);
    }

    const auto activated =
        activationService_.activateDispatching(
            claimed.operation.operationId);
    if (!activated.ok())
    {
        if (activated.status ==
            vdrsuite::agent::
                BackendAgentNativeTimerCreateActivationStatus::
                    providerSelectionStale)
        {
            return result(
                DaemonTimerCreateSubmissionStatus::
                    capabilityUnavailable,
                claimed.operation);
        }
        return result(
            DaemonTimerCreateSubmissionStatus::unavailable,
            claimed.operation);
    }

    const auto finalOperation =
        operationRepository_.findById(
            claimed.operation.operationId);
    return finalOperation.ok()
        ? result(
            DaemonTimerCreateSubmissionStatus::accepted,
            finalOperation.operation)
        : result(
            DaemonTimerCreateSubmissionStatus::accepted,
            claimed.operation);
}
