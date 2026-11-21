#include "BackendAgentNativeTimerCreateActivation.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace vdrsuite::agent
{
namespace
{
BackendAgentNativeTimerCreateActivationResult result(
    BackendAgentNativeTimerCreateActivationStatus status,
    const std::string& reasonCode,
    const BackendAgentCommandAssignment& assignment = {})
{
    BackendAgentNativeTimerCreateActivationResult value;
    value.status = status;
    value.reasonCode = reasonCode;
    value.assignment = assignment;
    return value;
}

bool nextRevision(const std::string& before, const std::string& after)
{
    if (before.empty() || after.empty()) return false;
    std::uint64_t value = 0;
    for (char character : before)
    {
        if (character < '0' || character > '9') return false;
        const std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    if (value == std::numeric_limits<std::uint64_t>::max()) return false;
    return after == std::to_string(value + 1);
}

bool sameActiveAssignment(
    BackendAgentCommandRepository& repository,
    const BackendAgentCommandReservation& reservation,
    const BackendAgentCommandAssignment& active)
{
    if (active.commandId != reservation.assignment.commandId ||
        active.requestFingerprint != reservation.assignment.requestFingerprint ||
        active.operationId != reservation.assignment.operationId ||
        active.jobId != reservation.assignment.jobId ||
        active.attemptId != reservation.assignment.attemptId ||
        active.claimEpoch != reservation.assignment.claimEpoch ||
        active.backendId != reservation.assignment.backendId ||
        active.agentId != reservation.assignment.agentId ||
        active.agentInstanceId != reservation.assignment.agentInstanceId ||
        active.backendGeneration != reservation.assignment.backendGeneration ||
        active.commandType != reservation.assignment.commandType ||
        active.payload != reservation.assignment.payload ||
        !reservation.localProviderSelectionPresent)
        return false;

    const auto activeSelection = repository.localProviderSelectionForCommand(
        active.commandId);
    return activeSelection.has_value() &&
        backendAgentLocalProviderSameFence(
            *activeSelection, reservation.localProviderSelection) &&
        backendAgentLocalProviderSelectionIdentity(*activeSelection) ==
            reservation.localProviderSelectionIdentity;
}
}

BackendAgentNativeTimerCreateActivationService::
BackendAgentNativeTimerCreateActivationService(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    BackendAgentCommandReservationRepository& reservationRepository,
    BackendAgentCommandRepository& commandRepository)
    : operationRepository_(operationRepository),
      reservationRepository_(reservationRepository),
      commandRepository_(commandRepository)
{
}

BackendAgentNativeTimerCreateActivationResult
BackendAgentNativeTimerCreateActivationService::activateDispatching(
    const std::string& operationId)
{
    using namespace vdrsuite::operations;
    using namespace vdrsuite::timers;

    if (!backendAgentCommandSafeIdentifier(operationId))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::invalid,
            "invalid_native_timer_create_activation_request");

    const auto foundOperation = operationRepository_.findById(operationId);
    if (foundOperation.status == MutationOperationRepositoryStatus::notFound)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::operationNotFound,
            "native_timer_create_operation_not_found");
    if (!foundOperation.ok())
        return result(
            BackendAgentNativeTimerCreateActivationStatus::storageError,
            "native_timer_create_operation_lookup_failed");
    const auto& operation = foundOperation.operation;
    if (operation.state != MutationOperationState::dispatching ||
        operation.actionFamily != "timer.create" ||
        operation.resourceType != "TimerAssignment" ||
        operation.verificationPolicy !=
            MutationOperationVerificationPolicy::readbackRequired)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::operationNotDispatching,
            "native_timer_create_operation_not_dispatching");

    NativeTimerCreateCommandReservationReference dispatchReference;
    if (!parseNativeTimerCreateCommandReservationReference(
            operation.resultReference, dispatchReference))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::dispatchReferenceInvalid,
            "native_timer_create_dispatch_reference_invalid");

    const auto foundReservation = reservationRepository_.findByCommandId(
        dispatchReference.commandId);
    if (foundReservation.status == BackendAgentCommandReservationStatus::notFound)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::reservationNotFound,
            "native_timer_create_reservation_not_found");
    if (!foundReservation.ok())
        return result(
            BackendAgentNativeTimerCreateActivationStatus::storageError,
            "native_timer_create_reservation_lookup_failed");
    const auto& reservation = foundReservation.reservation;
    const auto& assignment = reservation.assignment;

    if (dispatchReference.requestFingerprint != assignment.requestFingerprint ||
        assignment.operationId != operation.operationId ||
        assignment.backendId != operation.backendId ||
        assignment.backendGeneration != operation.backendGeneration ||
        assignment.commandType != kBackendAgentNativeTimerCreateCommandType ||
        assignment.payloadVersion != kBackendAgentNativeTimerCreatePayloadVersion ||
        assignment.verificationPolicy != "readback_required" ||
        !reservation.localProviderSelectionPresent ||
        reservation.localProviderSelectionIdentity !=
            backendAgentLocalProviderSelectionIdentity(
                reservation.localProviderSelection))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::identityConflict,
            "native_timer_create_activation_identity_conflict");

    BackendAgentNativeTimerCreatePayload agentPayload;
    std::string payloadReason;
    if (!backendAgentNativeTimerCreateParsePayload(
            assignment.payload, agentPayload, payloadReason) ||
        !backendAgentLocalProviderSameFence(
            agentPayload.localProviderSelection,
            reservation.localProviderSelection) ||
        !nextRevision(agentPayload.operationRevision, operation.operationRevision))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::payloadConflict,
            "native_timer_create_activation_payload_conflict");

    const auto foundPayload = operationRepository_.findPayloadByOperationId(operationId);
    if (!foundPayload.ok() ||
        foundPayload.payload.payloadType != "native.timer.create" ||
        foundPayload.payload.payloadVersion != 1)
        return result(
            foundPayload.status == MutationOperationRepositoryStatus::notFound
                ? BackendAgentNativeTimerCreateActivationStatus::payloadConflict
                : BackendAgentNativeTimerCreateActivationStatus::storageError,
            foundPayload.status == MutationOperationRepositoryStatus::notFound
                ? "native_timer_create_operation_payload_missing"
                : "native_timer_create_operation_payload_lookup_failed");

    NativeTimerCreateOperationPayload operationPayload;
    if (!parseNativeTimerCreateOperationPayload(
            foundPayload.payload.payload, operationPayload) ||
        nativeTimerCreateOperationPayloadFingerprint(operationPayload) !=
            foundPayload.payload.payloadFingerprint)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::payloadConflict,
            "native_timer_create_operation_payload_invalid");

    const std::string specificationFingerprint =
        nativeTimerSpecificationFingerprint(operationPayload.expectedSpecification);
    if (operation.resourceId != operationPayload.timerAssignmentId ||
        operation.expectedResourceFingerprint != specificationFingerprint ||
        operationPayload.timerAssignmentId != agentPayload.timerAssignmentId ||
        operationPayload.expectedAssignmentRevision !=
            agentPayload.expectedAssignmentRevision ||
        operationPayload.expectedIntentRevision != agentPayload.expectedIntentRevision ||
        operationPayload.assignmentEpoch != agentPayload.assignmentEpoch ||
        operationPayload.nativeTimerBindingId != agentPayload.nativeTimerBindingId ||
        operationPayload.backendId != assignment.backendId ||
        operationPayload.backendGeneration != assignment.backendGeneration ||
        specificationFingerprint.empty() ||
        specificationFingerprint != agentPayload.expectedSpecificationFingerprint ||
        specificationFingerprint !=
            backendAgentNativeTimerCreateSpecificationFingerprint(
                agentPayload.specification))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::payloadConflict,
            "native_timer_create_activation_payload_conflict");

    const auto active = commandRepository_.findAssignmentForOperation(
        assignment.backendId, operationId, kBackendAgentNativeTimerCreateCommandType);
    if (active.has_value())
    {
        if (!sameActiveAssignment(commandRepository_, reservation, *active))
            return result(
                BackendAgentNativeTimerCreateActivationStatus::activationConflict,
                "native_timer_create_active_assignment_conflict",
                *active);
        return result(
            BackendAgentNativeTimerCreateActivationStatus::alreadyActivated,
            "native_timer_create_reservation_already_activated",
            *active);
    }

    std::string providerReason;
    const auto currentSelection = commandRepository_.selectLocalProvider(
        assignment.backendId,
        assignment.agentId,
        assignment.agentInstanceId,
        assignment.backendGeneration,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability,
        providerReason);
    if (!currentSelection.has_value() ||
        !backendAgentLocalProviderSameFence(
            *currentSelection, reservation.localProviderSelection))
        return result(
            BackendAgentNativeTimerCreateActivationStatus::providerSelectionStale,
            providerReason.empty()
                ? "native_timer_create_provider_selection_stale"
                : providerReason);

    BackendAgentCommandReservationActivationService activation(
        reservationRepository_, commandRepository_);
    const auto activated = activation.activate(dispatchReference.commandId);
    if (activated.status == BackendAgentCommandReservationStatus::activated)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::activated,
            "native_timer_create_reservation_activated",
            activated.assignment);
    if (activated.status == BackendAgentCommandReservationStatus::alreadyActivated)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::alreadyActivated,
            "native_timer_create_reservation_already_activated",
            activated.assignment);
    if (activated.status == BackendAgentCommandReservationStatus::conflict)
        return result(
            BackendAgentNativeTimerCreateActivationStatus::activationConflict,
            "native_timer_create_activation_conflict",
            activated.assignment);
    return result(
        activated.status == BackendAgentCommandReservationStatus::notFound
            ? BackendAgentNativeTimerCreateActivationStatus::reservationNotFound
            : BackendAgentNativeTimerCreateActivationStatus::storageError,
        activated.status == BackendAgentCommandReservationStatus::notFound
            ? "native_timer_create_reservation_not_found"
            : "native_timer_create_activation_failed");
}

} // namespace vdrsuite::agent
