#include "NativeTimerDeleteProductiveRuntime.h"
#include "NativeTimerDeleteResultOutcomeApplication.h"
#include "NativeTimerDeletePublicPreflight.h"
#include "NativeTimerDeleteDispatchService.h"
#include "NativeTimerDeleteOperationCompletionService.h"
#include "NativeTimerAbsenceReadbackVerificationService.h"
#include "NativeTimerBindingRepository.h"
#include "MutationOperationRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteAssignment.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::daemon {
namespace {
using namespace vdrsuite::timers;
using namespace vdrsuite::operations;
using namespace vdrsuite::agent;

RequestSecurityContext systemContext() {
    RequestSecurityContext ctx;
    ctx.requestId = "req:native-timer-delete-runtime";
    ctx.correlationId = ctx.requestId;
    ctx.authenticationState = AuthenticationState::Authenticated;
    ctx.actor = ActorIdentity{"system:native-timer-delete-runtime",
        ActorType::System, "Native Timer DELETE runtime", true};
    ctx.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return ctx;
}

bool assignThenClaim(
    const MutationOperation& operation,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    NativeTimerBindingRepository& bindings,
    BackendAgentNativeTimerDeleteAssignmentService& assigner,
    NativeTimerDeleteDispatchService& dispatch,
    std::int64_t now)
{
    if (operation.actionFamily != "timer.delete" ||
        operation.resourceType != "NativeTimerBinding" ||
        operation.verificationPolicy !=
            MutationOperationVerificationPolicy::readbackRequired ||
        operation.deadline != 0 && operation.deadline <= now ||
        now > std::numeric_limits<std::int64_t>::max() - 300)
        return false;

    const auto found = bindings.findById(operation.resourceId);
    if (!found.ok()) return false;
    const auto& binding = found.binding;
    const auto agent = agents.findAgentForBackend(operation.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->leaseExpiresAt < now ||
        agent->backendGeneration != operation.backendGeneration)
        return false;

    PublicNativeTimerDeletePreflightRequest preflight;
    preflight.nativeTimerBindingId = binding.nativeTimerBindingId;
    preflight.backendId = operation.backendId;
    preflight.backendNativeTimerId = binding.backendNativeTimerId;
    preflight.expectedBackendGeneration = operation.backendGeneration;
    preflight.expectedBindingRevision = operation.expectedRevision;
    preflight.expectedObservedFingerprint =
        operation.expectedResourceFingerprint;
    preflight.observedNotBefore = std::max<std::int64_t>(1, now - 300);
    if (publicNativeTimerDeletePreflight(preflight, binding) !=
        PublicNativeTimerDeletePreflightStatus::eligible)
        return false;

    const auto previous = commands.findAssignmentForOperation(
        operation.backendId, operation.operationId,
        kBackendAgentNativeTimerDeleteCommandType);
    std::int64_t assignedAt = now;
    if (previous) {
        BackendAgentNativeTimerDeletePayload payload;
        std::string reason;
        if (!backendAgentNativeTimerDeleteParsePayload(
                previous->payload, payload, reason) ||
            previous->operationId != operation.operationId ||
            previous->backendGeneration != operation.backendGeneration ||
            payload.operationRevision != operation.operationRevision ||
            payload.nativeTimerBindingId != binding.nativeTimerBindingId ||
            payload.expectedBindingRevision != operation.expectedRevision ||
            payload.timerAssignmentId != binding.timerAssignmentId ||
            payload.backendNativeTimerId != binding.backendNativeTimerId ||
            payload.controlPlaneClaimedAt != previous->assignedAt)
            return false;
        assignedAt = previous->assignedAt;
    }

    BackendAgentNativeTimerDeleteAssignmentRequest request;
    request.operationId = operation.operationId;
    request.operationRevision = operation.operationRevision;
    request.nativeTimerBindingId = binding.nativeTimerBindingId;
    request.expectedBindingRevision = operation.expectedRevision;
    request.expectedNativeTimerFingerprint =
        operation.expectedResourceFingerprint;
    request.timerAssignmentId = binding.timerAssignmentId;
    request.backendId = operation.backendId;
    request.backendGeneration = operation.backendGeneration;
    request.backendNativeTimerId = binding.backendNativeTimerId;
    request.controlPlaneClaimedAt = assignedAt;
    const auto assigned = assigner.assign(
        systemContext(), request, now, now + 300);
    if (!assigned.accepted ||
        assigned.assignment.assignedAt != assignedAt)
        return false;

    NativeTimerDeleteDispatchHandoff handoff;
    handoff.operationId = operation.operationId;
    handoff.operationRevision = operation.operationRevision;
    handoff.nativeTimerBindingId = binding.nativeTimerBindingId;
    handoff.expectedBindingRevision = operation.expectedRevision;
    handoff.expectedNativeTimerFingerprint =
        operation.expectedResourceFingerprint;
    handoff.timerAssignmentId = binding.timerAssignmentId;
    handoff.backendId = operation.backendId;
    handoff.backendGeneration = operation.backendGeneration;
    handoff.backendNativeTimerId = binding.backendNativeTimerId;
    return dispatch.claim(handoff, assignedAt).ok();
}
} // namespace

NativeTimerDeleteProductiveRuntimeSummary advanceNativeTimerDeleteRuntimeOnce(
    MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    NativeTimerBindingRepository& bindings,
    BackendAgentNativeTimerDeleteAssignmentService& assigner,
    NativeTimerDeleteDispatchService& dispatch,
    NativeTimerAbsenceReadbackVerificationService& verification,
    NativeTimerDeleteOperationCompletionService& completion,
    const NativeDeleteReadbackAcquirer& readback,
    std::int64_t now)
{
    NativeTimerDeleteProductiveRuntimeSummary summary;
    if (now <= 0 || !readback) return summary;
    const auto candidates = operations.listByActionFamilyAndStates(
        "timer.delete",
        {MutationOperationState::accepted,
         MutationOperationState::dispatching,
         MutationOperationState::executedUnverified,
         MutationOperationState::outcomeUnknown}, 64);
    if (!candidates.ok()) return summary;

    summary.discovered = candidates.operations.size();
    for (const auto& candidate : candidates.operations) {
        if (candidate.state == MutationOperationState::accepted) {
            if (!assignThenClaim(candidate, commands, agents, bindings,
                    assigner, dispatch, now)) {
                ++summary.deferred;
                continue;
            }
            ++summary.dispatched;
        }
        const auto command = commands.findAssignmentForOperation(
            candidate.backendId, candidate.operationId,
            kBackendAgentNativeTimerDeleteCommandType);
        if (!command || !commands.resultForCommand(command->commandId)) {
            ++summary.deferred;
            continue;
        }

        const auto outcome = applyDurableNativeTimerDeleteResult(
            commands, operations, bindings, dispatch, command->commandId);
        if (!outcome.ok()) {
            ++summary.deferred;
            continue;
        }
        if (outcome.status == NativeTimerDeleteResultOutcomeStatus::applied)
            ++summary.outcomes;
        if (!outcome.dispatch.expectationPresent)
            continue;

        const auto evidence = readback(outcome.dispatch.expectation);
        if (!evidence) {
            ++summary.deferred;
            continue;
        }
        const auto verified = verification.verify(
            outcome.dispatch.expectation, *evidence);
        if (!verified.ok()) {
            ++summary.deferred;
            continue;
        }
        const auto finished = completion.complete(
            outcome.dispatch.expectation, now);
        if (finished.ok()) {
            if (finished.status ==
                    NativeTimerDeleteOperationCompletionStatus::completed)
                ++summary.completed;
        } else {
            ++summary.deferred;
        }
    }
    return summary;
}
} // namespace vdrsuite::daemon
