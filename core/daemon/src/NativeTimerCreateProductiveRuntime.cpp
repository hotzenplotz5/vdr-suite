#include "NativeTimerCreateProductiveRuntime.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateActivation.h"
#include "BackendAgentNativeTimerCreateReservation.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateReadbackReconciliation.h"
#include "NativeTimerCreateResultOutcomeApplication.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerSpecification.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace vdrsuite::daemon
{
namespace
{
using namespace vdrsuite::agent;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

constexpr std::size_t kMaximumRuntimeCandidates = 64;
constexpr std::int64_t kAgentCommandLifetimeSeconds = 300;

RequestSecurityContext systemContext()
{
    RequestSecurityContext context;
    context.requestId = "req:native-timer-create-runtime";
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:native-timer-create-runtime",
        ActorType::System,
        "Native Timer CREATE runtime",
        true};
    context.permissionGrantResolution =
        PermissionGrantResolutionState::Resolved;
    return context;
}

BackendAgentNativeTimerCreateSpecification agentSpecification(
    const NativeTimerSpecification& specification)
{
    BackendAgentNativeTimerCreateSpecification value;
    value.channelId = specification.channelId;
    value.title = specification.title;
    value.directory = specification.directory;
    value.day = specification.day;
    value.weekdays = specification.weekdays;
    value.startTime = specification.startTime;
    value.endTime = specification.endTime;
    value.priority = specification.priority;
    value.lifetime = specification.lifetime;
    value.enabled = specification.enabled;
    value.vps = specification.vps;
    return value;
}

bool loadPayload(
    MutationOperationRepository& operationRepository,
    const MutationOperation& operation,
    NativeTimerCreateOperationPayload& payload)
{
    const auto durable =
        operationRepository.findPayloadByOperationId(operation.operationId);
    if (!durable.ok() ||
        durable.payload.payloadType != "native.timer.create" ||
        durable.payload.payloadVersion != 1 ||
        !parseNativeTimerCreateOperationPayload(
            durable.payload.payload, payload) ||
        durable.payload.payload !=
            serializeNativeTimerCreateOperationPayload(payload) ||
        durable.payload.payloadFingerprint !=
            nativeTimerCreateOperationPayloadFingerprint(payload))
    {
        return false;
    }

    const std::string specificationFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    return operation.actionFamily == "timer.create" &&
        operation.resourceType == "TimerAssignment" &&
        operation.resourceId == payload.timerAssignmentId &&
        operation.expectedRevision == payload.expectedAssignmentRevision &&
        operation.backendId == payload.backendId &&
        operation.backendGeneration == payload.backendGeneration &&
        operation.expectedResourceFingerprint == specificationFingerprint &&
        operation.verificationPolicy ==
            MutationOperationVerificationPolicy::readbackRequired &&
        !specificationFingerprint.empty();
}

std::int64_t commandDeadline(
    const MutationOperation& operation,
    std::int64_t now)
{
    if (now <= 0 ||
        now > std::numeric_limits<std::int64_t>::max() -
            kAgentCommandLifetimeSeconds)
    {
        return 0;
    }
    const std::int64_t bounded = now + kAgentCommandLifetimeSeconds;
    if (operation.deadline == 0)
        return bounded;
    if (operation.deadline <= now)
        return 0;
    return std::min(operation.deadline, bounded);
}

bool reserveClaimAndActivate(
    BackendAgentNativeTimerCreateReservationService& reservationService,
    NativeTimerCreateDispatchService& dispatchService,
    BackendAgentNativeTimerCreateActivationService& activationService,
    const MutationOperation& accepted,
    const NativeTimerCreateOperationPayload& payload,
    std::int64_t now,
    NativeTimerCreateProductiveRuntimeSummary& summary)
{
    const auto nativeSpecification = agentSpecification(
        payload.expectedSpecification);
    const std::string agentFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(
            nativeSpecification);
    const std::string domainFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    const std::int64_t deadline = commandDeadline(accepted, now);

    if (deadline == 0 || agentFingerprint.empty() ||
        agentFingerprint != domainFingerprint ||
        domainFingerprint != accepted.expectedResourceFingerprint)
    {
        ++summary.deferred;
        return false;
    }

    BackendAgentNativeTimerCreateReservationRequest reservationRequest;
    reservationRequest.operationId = accepted.operationId;
    reservationRequest.operationRevision = accepted.operationRevision;
    reservationRequest.timerAssignmentId = payload.timerAssignmentId;
    reservationRequest.expectedAssignmentRevision =
        payload.expectedAssignmentRevision;
    reservationRequest.expectedIntentRevision =
        payload.expectedIntentRevision;
    reservationRequest.assignmentEpoch = payload.assignmentEpoch;
    reservationRequest.nativeTimerBindingId =
        payload.nativeTimerBindingId;
    reservationRequest.backendId = payload.backendId;
    reservationRequest.backendGeneration = payload.backendGeneration;
    reservationRequest.expectedSpecification = nativeSpecification;
    reservationRequest.expectedSpecificationFingerprint = agentFingerprint;

    const auto reserved = reservationService.reserve(
        systemContext(), reservationRequest, now, deadline);
    if (!reserved.accepted)
    {
        ++summary.deferred;
        return false;
    }

    NativeTimerCreateDispatchClaimRequest claim;
    claim.operationId = accepted.operationId;
    claim.expectedOperationRevision = accepted.operationRevision;
    claim.timerAssignmentId = payload.timerAssignmentId;
    claim.nativeTimerBindingId = payload.nativeTimerBindingId;
    claim.backendId = payload.backendId;
    claim.backendGeneration = payload.backendGeneration;
    claim.expectedSpecificationFingerprint = domainFingerprint;
    claim.reservation.commandId = reserved.assignment.commandId;
    claim.reservation.requestFingerprint =
        reserved.assignment.requestFingerprint;

    const auto claimed =
        dispatchService.claimAfterReservation(claim, now);
    if (!claimed.ok())
    {
        ++summary.deferred;
        return false;
    }
    if (claimed.status == NativeTimerCreateDispatchClaimStatus::claimed)
        ++summary.dispatchClaims;

    const auto activated =
        activationService.activateDispatching(accepted.operationId);
    if (!activated.ok())
    {
        ++summary.deferred;
        return false;
    }
    if (activated.status ==
        BackendAgentNativeTimerCreateActivationStatus::activated)
    {
        ++summary.activations;
    }
    return true;
}

std::optional<std::string> activeCommandId(
    BackendAgentCommandRepository& commandRepository,
    const MutationOperation& operation)
{
    const auto assignment = commandRepository.findAssignmentForOperation(
        operation.backendId,
        operation.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    if (!assignment.has_value())
        return std::nullopt;
    return assignment->commandId;
}
} // namespace

NativeTimerCreateProductiveRuntimeSummary
advanceNativeTimerCreateRuntimeOnce(
    MutationOperationRepository& operationRepository,
    BackendAgentCommandRepository& commandRepository,
    BackendAgentNativeTimerCreateReservationService& reservationService,
    NativeTimerCreateDispatchService& dispatchService,
    BackendAgentNativeTimerCreateActivationService& activationService,
    NativeTimerCreateReadbackVerificationService& verificationService,
    TimerAssignmentFulfillmentService& fulfillmentService,
    NativeTimerCreateOperationCompletionService& completionService,
    const NativeTimerCreateReadbackAcquirer& readbackAcquirer,
    std::int64_t now)
{
    NativeTimerCreateProductiveRuntimeSummary summary;
    if (now <= 0 || !readbackAcquirer)
        return summary;

    const auto candidates =
        operationRepository.listByActionFamilyAndStates(
            "timer.create",
            {
                MutationOperationState::accepted,
                MutationOperationState::dispatching,
                MutationOperationState::executedUnverified,
                MutationOperationState::outcomeUnknown,
            },
            kMaximumRuntimeCandidates);
    if (!candidates.ok())
        return summary;

    summary.discovered = candidates.operations.size();

    for (const auto& candidate : candidates.operations)
    {
        NativeTimerCreateOperationPayload payload;
        if (!loadPayload(operationRepository, candidate, payload))
        {
            ++summary.deferred;
            continue;
        }

        MutationOperation current = candidate;
        if (current.state == MutationOperationState::accepted)
        {
            if (!reserveClaimAndActivate(
                    reservationService,
                    dispatchService,
                    activationService,
                    current,
                    payload,
                    now,
                    summary))
            {
                continue;
            }

            const auto refreshed =
                operationRepository.findById(current.operationId);
            if (!refreshed.ok())
            {
                ++summary.deferred;
                continue;
            }
            current = refreshed.operation;
        }
        else if (current.state == MutationOperationState::dispatching)
        {
            const auto activated =
                activationService.activateDispatching(current.operationId);
            if (!activated.ok())
            {
                ++summary.deferred;
                continue;
            }
            if (activated.status ==
                BackendAgentNativeTimerCreateActivationStatus::activated)
            {
                ++summary.activations;
            }
        }

        if (current.state == MutationOperationState::dispatching ||
            current.state == MutationOperationState::executedUnverified ||
            current.state == MutationOperationState::outcomeUnknown)
        {
            const auto commandId =
                activeCommandId(commandRepository, current);
            if (!commandId.has_value() ||
                !commandRepository.resultForCommand(*commandId).has_value())
            {
                ++summary.deferred;
                continue;
            }

            const auto outcome = applyDurableNativeTimerCreateResult(
                commandRepository,
                operationRepository,
                dispatchService,
                *commandId);
            if (!outcome.ok())
            {
                ++summary.deferred;
                continue;
            }
            if (outcome.status ==
                NativeTimerCreateResultOutcomeApplicationStatus::applied)
            {
                ++summary.outcomesApplied;
            }

            if (!outcome.dispatch.expectationPresent)
                continue;

            const auto readback =
                readbackAcquirer(outcome.dispatch.expectation);
            if (!readback.has_value())
            {
                ++summary.deferred;
                continue;
            }

            const auto reconciled = reconcileNativeTimerCreateReadback(
                operationRepository,
                verificationService,
                fulfillmentService,
                completionService,
                outcome.dispatch.expectation,
                *readback,
                now);
            if (!reconciled.ok())
            {
                ++summary.deferred;
                continue;
            }
            if (reconciled.status ==
                NativeTimerCreateReadbackReconciliationStatus::completed)
            {
                ++summary.reconciliationsCompleted;
            }
        }
    }

    return summary;
}

} // namespace vdrsuite::daemon
