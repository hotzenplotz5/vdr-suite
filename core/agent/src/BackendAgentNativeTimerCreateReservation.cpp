#include "BackendAgentNativeTimerCreateReservation.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreatePayload.h"

#include <optional>
#include <string>

namespace
{
using namespace vdrsuite::agent;

BackendAgentNativeTimerCreateReservationResult rejected(
    const std::string& reasonCode)
{
    BackendAgentNativeTimerCreateReservationResult result;
    result.reasonCode = reasonCode;
    return result;
}

bool requestValid(
    const RequestSecurityContext& context,
    const BackendAgentNativeTimerCreateReservationRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    return context.authenticated() && context.actor.type == ActorType::System &&
        backendAgentCommandSafeIdentifier(request.operationId) &&
        backendAgentCommandSafeIdentifier(request.operationRevision) &&
        backendAgentCommandSafeIdentifier(request.timerAssignmentId) &&
        backendAgentCommandSafeIdentifier(request.expectedAssignmentRevision) &&
        backendAgentCommandSafeIdentifier(request.expectedIntentRevision) &&
        request.assignmentEpoch > 0 &&
        backendAgentCommandSafeIdentifier(request.nativeTimerBindingId) &&
        backendAgentCommandSafeIdentifier(request.backendId) &&
        request.backendGeneration > 0 &&
        backendAgentNativeTimerCreateSpecificationValid(request.expectedSpecification) &&
        request.expectedSpecificationFingerprint ==
            backendAgentNativeTimerCreateSpecificationFingerprint(
                request.expectedSpecification) &&
        now > 0 && deadline > now && deadline - now <= 3600;
}

BackendAgentNativeTimerCreateCommand domainCommand(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreatePayload& payload)
{
    BackendAgentNativeTimerCreateCommand command;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = payload.operationRevision;
    command.timerAssignmentId = payload.timerAssignmentId;
    command.expectedAssignmentRevision = payload.expectedAssignmentRevision;
    command.expectedIntentRevision = payload.expectedIntentRevision;
    command.assignmentEpoch = payload.assignmentEpoch;
    command.nativeTimerBindingId = payload.nativeTimerBindingId;
    command.expectedSpecificationFingerprint = payload.expectedSpecificationFingerprint;
    command.jobId = assignment.jobId;
    command.attemptId = assignment.attemptId;
    command.claimEpoch = assignment.claimEpoch;
    command.backendId = assignment.backendId;
    command.agentId = assignment.agentId;
    command.agentInstanceId = assignment.agentInstanceId;
    command.backendGeneration = assignment.backendGeneration;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.specification = payload.specification;
    command.localProviderSelection = payload.localProviderSelection;
    return command;
}

bool requestMatches(
    const BackendAgentNativeTimerCreateReservationRequest& request,
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreatePayload& payload)
{
    return assignment.commandType == kBackendAgentNativeTimerCreateCommandType &&
        assignment.payloadVersion == kBackendAgentNativeTimerCreatePayloadVersion &&
        assignment.verificationPolicy == "readback_required" &&
        assignment.operationId == request.operationId &&
        assignment.backendId == request.backendId &&
        assignment.backendGeneration == request.backendGeneration &&
        payload.operationRevision == request.operationRevision &&
        payload.timerAssignmentId == request.timerAssignmentId &&
        payload.expectedAssignmentRevision == request.expectedAssignmentRevision &&
        payload.expectedIntentRevision == request.expectedIntentRevision &&
        payload.assignmentEpoch == request.assignmentEpoch &&
        payload.nativeTimerBindingId == request.nativeTimerBindingId &&
        payload.expectedSpecificationFingerprint ==
            request.expectedSpecificationFingerprint &&
        backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification) ==
            request.expectedSpecificationFingerprint;
}

bool exactExistingReservation(
    BackendAgentCommandRepository& commandRepository,
    const BackendAgentRecord& agent,
    const BackendAgentNativeTimerCreateReservationRequest& request,
    const BackendAgentCommandReservation& reservation,
    std::string& reasonCode)
{
    const auto& assignment = reservation.assignment;
    BackendAgentNativeTimerCreatePayload payload;
    if (!backendAgentNativeTimerCreateParsePayload(
            assignment.payload, payload, reasonCode) ||
        !requestMatches(request, assignment, payload) ||
        !backendAgentCommandValidAssignment(assignment))
    {
        reasonCode = "native_timer_create_reservation_conflict";
        return false;
    }
    if (assignment.agentId != agent.agentId ||
        assignment.agentInstanceId != agent.agentInstanceId ||
        assignment.backendGeneration != agent.backendGeneration)
    {
        reasonCode = "native_timer_create_agent_fence_stale";
        return false;
    }
    if (!reservation.localProviderSelectionPresent ||
        reservation.localProviderSelectionIdentity !=
            backendAgentLocalProviderSelectionIdentity(payload.localProviderSelection) ||
        !backendAgentLocalProviderSameFence(
            reservation.localProviderSelection, payload.localProviderSelection))
    {
        reasonCode = "native_timer_create_provider_selection_missing";
        return false;
    }

    const auto current = commandRepository.selectLocalProvider(
        request.backendId,
        agent.agentId,
        agent.agentInstanceId,
        agent.backendGeneration,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability,
        reasonCode);
    if (!current.has_value()) return false;
    if (!backendAgentLocalProviderSameFence(*current, payload.localProviderSelection))
    {
        reasonCode = "native_timer_create_provider_selection_stale";
        return false;
    }

    const auto command = domainCommand(assignment, payload);
    if (!backendAgentNativeTimerCreateValidCommand(command, reasonCode))
    {
        reasonCode = "native_timer_create_reservation_contract_invalid";
        return false;
    }
    return true;
}

BackendAgentNativeTimerCreateReservationResult accepted(
    const BackendAgentCommandAssignment& assignment,
    bool replayed,
    const std::string& reasonCode)
{
    BackendAgentNativeTimerCreateReservationResult result;
    result.accepted = true;
    result.replayed = replayed;
    result.reasonCode = reasonCode;
    result.assignment = assignment;
    return result;
}
}

namespace vdrsuite::agent
{

BackendAgentNativeTimerCreateReservationService::
BackendAgentNativeTimerCreateReservationService(
    BackendAgentCommandRepository& commandRepository,
    BackendAgentCommandReservationRepository& reservationRepository,
    BackendAgentRepository& agentRepository)
    : commandRepository_(commandRepository),
      reservationRepository_(reservationRepository),
      agentRepository_(agentRepository)
{
}

BackendAgentNativeTimerCreateReservationResult
BackendAgentNativeTimerCreateReservationService::reserve(
    const RequestSecurityContext& context,
    const BackendAgentNativeTimerCreateReservationRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    if (!requestValid(context, request, now, deadline))
        return rejected("invalid_native_timer_create_reservation_request");

    const auto agent = agentRepository_.findAgentForBackend(request.backendId);
    if (!agent.has_value() || agent->revoked || agent->incompatible ||
        agent->agentInstanceId.empty() || agent->backendGeneration == 0 ||
        agent->leaseExpiresAt < now)
        return rejected("active_agent_lease_required");
    if (agent->backendGeneration != request.backendGeneration)
        return rejected("native_timer_create_backend_generation_conflict");

    if (!reservationRepository_.ensureSchema())
        return rejected("native_timer_create_reservation_schema_failed");

    const auto existing = reservationRepository_.findForOperation(
        request.backendId,
        request.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    if (existing.ok())
    {
        std::string reason;
        if (!exactExistingReservation(
                commandRepository_, *agent, request, existing.reservation, reason))
            return rejected(reason);
        return accepted(
            existing.reservation.assignment,
            true,
            "native_timer_create_reservation_replayed");
    }
    if (existing.status != BackendAgentCommandReservationStatus::notFound)
    {
        return rejected(
            existing.status == BackendAgentCommandReservationStatus::invalid
                ? "invalid_native_timer_create_reservation_request"
                : "native_timer_create_reservation_lookup_failed");
    }

    if (commandRepository_.findAssignmentForOperation(
            request.backendId,
            request.operationId,
            kBackendAgentNativeTimerCreateCommandType).has_value())
        return rejected("native_timer_create_active_assignment_conflict");

    std::string reason;
    const auto selection = commandRepository_.selectLocalProvider(
        request.backendId,
        agent->agentId,
        agent->agentInstanceId,
        agent->backendGeneration,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability,
        reason);
    if (!selection.has_value()) return rejected(reason);

    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = request.operationRevision;
    payload.timerAssignmentId = request.timerAssignmentId;
    payload.expectedAssignmentRevision = request.expectedAssignmentRevision;
    payload.expectedIntentRevision = request.expectedIntentRevision;
    payload.assignmentEpoch = request.assignmentEpoch;
    payload.nativeTimerBindingId = request.nativeTimerBindingId;
    payload.controlPlaneClaimedAt = now;
    payload.expectedSpecificationFingerprint = request.expectedSpecificationFingerprint;
    payload.specification = request.expectedSpecification;
    payload.localProviderSelection = *selection;

    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = backendAgentGenerateOpaqueId("req_", 8);
    assignment.correlationId = assignment.requestId;
    assignment.operationId = request.operationId;
    assignment.jobId = backendAgentGenerateOpaqueId("job_", 12);
    assignment.attemptId = backendAgentGenerateOpaqueId("att_", 12);
    assignment.claimEpoch = 1;
    assignment.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
    assignment.backendId = request.backendId;
    assignment.agentId = agent->agentId;
    assignment.agentInstanceId = agent->agentInstanceId;
    assignment.backendGeneration = agent->backendGeneration;
    assignment.commandType = kBackendAgentNativeTimerCreateCommandType;
    assignment.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    assignment.payload = backendAgentNativeTimerCreatePayload(payload);
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = now;
    assignment.deadline = deadline;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);

    const auto command = domainCommand(assignment, payload);
    if (assignment.payload.empty() ||
        !backendAgentNativeTimerCreateValidCommand(command, reason) ||
        !backendAgentCommandValidAssignment(assignment))
        return rejected("native_timer_create_reservation_contract_invalid");

    const auto reserved = reservationRepository_.reserve(assignment, &*selection);
    if (reserved.status == BackendAgentCommandReservationStatus::reserved)
        return accepted(
            reserved.reservation.assignment,
            false,
            "native_timer_create_reserved");
    if (reserved.status == BackendAgentCommandReservationStatus::alreadyReserved)
    {
        if (!exactExistingReservation(
                commandRepository_, *agent, request, reserved.reservation, reason))
            return rejected(reason);
        return accepted(
            reserved.reservation.assignment,
            true,
            "native_timer_create_reservation_replayed");
    }
    if (reserved.status == BackendAgentCommandReservationStatus::conflict)
        return rejected("native_timer_create_reservation_conflict");
    return rejected(
        reserved.status == BackendAgentCommandReservationStatus::invalid
            ? "native_timer_create_reservation_contract_invalid"
            : "native_timer_create_reservation_persist_failed");
}

} // namespace vdrsuite::agent
