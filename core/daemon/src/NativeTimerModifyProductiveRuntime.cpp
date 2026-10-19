#include "NativeTimerModifyProductiveRuntime.h"

#include "NativeTimerModifyResultOutcomeApplication.h"
#include "NativeTimerModifyOperationPayload.h"
#include "NativeTimerModifyDispatchService.h"
#include "NativeTimerModifyReadbackVerificationService.h"
#include "NativeTimerModifyOperationCompletionService.h"
#include "NativeTimerBindingRepository.h"
#include "TimerIntentRepository.h"
#include "TimerAssignmentRepository.h"
#include "MutationOperationRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerModify.h"
#include "BackendAgentNativeTimerModifyPayload.h"
#include "BackendAgentNativeTimerModifyAssignment.h"
#include "BackendAgentNativeTimerCreate.h"
#include "SecurityIdentity.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::daemon {
namespace {
using namespace vdrsuite::timers;
using namespace vdrsuite::agent;
using namespace vdrsuite::operations;

RequestSecurityContext systemContext()
{
    RequestSecurityContext context;
    context.requestId = "req:native-timer-modify-runtime";
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:native-timer-modify-runtime", ActorType::System,
        "Native Timer MODIFY runtime", true};
    context.permissionGrantResolution =
        PermissionGrantResolutionState::Resolved;
    return context;
}

BackendAgentNativeTimerCreateSpecification agentSpecification(
    const NativeTimerSpecification& s)
{
    BackendAgentNativeTimerCreateSpecification v;
    v.channelId = s.channelId;
    v.title = s.title;
    v.directory = s.directory;
    v.day = s.day;
    v.weekdays = s.weekdays;
    v.startTime = s.startTime;
    v.endTime = s.endTime;
    v.priority = s.priority;
    v.lifetime = s.lifetime;
    v.enabled = s.enabled;
    v.vps = s.vps;
    return v;
}

bool assignAndClaim(
    const MutationOperation& op,
    MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    TimerIntentRepository& intents,
    TimerAssignmentRepository& assignments,
    NativeTimerBindingRepository& bindings,
    BackendAgentNativeTimerModifyAssignmentService& assigner,
    NativeTimerModifyDispatchService& dispatcher,
    std::int64_t now)
{
    if (op.state != MutationOperationState::accepted ||
        (op.actionFamily != "timer.update" &&
         op.actionFamily != "timer.toggle") ||
        op.resourceType != "NativeTimerBinding" ||
        op.verificationPolicy != MutationOperationVerificationPolicy::readbackRequired ||
        (op.deadline != 0 && op.deadline <= now) ||
        now <= 0 || now > std::numeric_limits<std::int64_t>::max() - 300)
        return false;

    const auto stored = operations.findPayloadByOperationId(op.operationId);
    if (!stored.ok() || stored.payload.payloadType != "native.timer.modify" ||
        stored.payload.payloadVersion != 1)
        return false;
    NativeTimerModifyOperationPayload payload;
    if (!parseNativeTimerModifyOperationPayload(stored.payload.payload, payload) ||
        nativeTimerModifyOperationPayloadFingerprint(payload) !=
            stored.payload.payloadFingerprint)
        return false;
    if (op.actionFamily !=
        (payload.kind == NativeTimerModifyKind::update
            ? "timer.update" : "timer.toggle"))
        return false;

    const auto owned = bindings.findById(payload.nativeTimerBindingId);
    const auto assigned = assignments.findById(payload.timerAssignmentId);
    if (!owned.ok() || !assigned.ok())
        return false;
    const auto intent = intents.findById(assigned.assignment.timerIntentId);
    if (!intent.ok()) return false;
    const auto& b = owned.binding;
    const auto& a = assigned.assignment;
    if (b.nativeTimerBindingId != op.resourceId ||
        b.timerAssignmentId != a.timerAssignmentId ||
        b.backendId != op.backendId ||
        b.backendGeneration != op.backendGeneration ||
        b.backendNativeTimerId != payload.backendNativeTimerId ||
        b.bindingRevision != payload.expectedBindingRevision ||
        b.observedFingerprint != payload.expectedCurrentFingerprint ||
        b.missingSince != 0 ||
        b.driftState != NativeTimerBindingDriftState::none ||
        b.observedState.recording || b.observedState.pending ||
        (b.ownership != NativeTimerBindingOwnership::managed &&
         b.ownership != NativeTimerBindingOwnership::adopted) ||
        a.state != TimerAssignmentState::bound ||
        a.nativeTimerBindingId != b.nativeTimerBindingId ||
        a.backendId != op.backendId ||
        a.backendGeneration != op.backendGeneration ||
        a.assignmentRevision != payload.expectedAssignmentRevision ||
        a.assignmentEpoch != payload.assignmentEpoch ||
        a.intentRevision != payload.expectedIntentRevision ||
        intent.intent.intentRevision != payload.expectedIntentRevision ||
        (intent.intent.state != TimerIntentState::active &&
         !(payload.kind == NativeTimerModifyKind::toggle &&
           intent.intent.state == TimerIntentState::paused)) ||
        op.expectedRevision != b.bindingRevision ||
        op.expectedResourceFingerprint != b.observedFingerprint)
        return false;

    const auto agent = agents.findAgentForBackend(op.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->leaseExpiresAt < now ||
        agent->backendGeneration != op.backendGeneration)
        return false;
    if (b.lastObservedAt <= 0 || b.lastObservedAt < now - 300)
        return false;

    const char* type = payload.kind == NativeTimerModifyKind::toggle
        ? kBackendAgentNativeTimerToggleCommandType
        : kBackendAgentNativeTimerUpdateCommandType;
    const auto previous = commands.findAssignmentForOperation(
        op.backendId, op.operationId, type);
    std::int64_t assignedAt = now;
    if (previous)
    {
        BackendAgentNativeTimerModifyPayload p;
        std::string reason;
        if (!backendAgentNativeTimerModifyParsePayload(
                previous->payload, p, reason) ||
            p.operationRevision != op.operationRevision ||
            p.timerAssignmentId != payload.timerAssignmentId ||
            p.expectedBindingRevision != payload.expectedBindingRevision ||
            p.backendNativeTimerId != payload.backendNativeTimerId ||
            p.expectedIntentRevision != payload.expectedIntentRevision ||
            p.expectedAssignmentRevision != payload.expectedAssignmentRevision ||
            previous->backendGeneration != op.backendGeneration ||
            p.controlPlaneClaimedAt != previous->assignedAt)
            return false;
        assignedAt = previous->assignedAt;
    }

    const auto specification = agentSpecification(payload.expectedSpecification);
    BackendAgentNativeTimerModifyAssignmentRequest request;
    request.kind = payload.kind == NativeTimerModifyKind::toggle
        ? BackendAgentNativeTimerModifyKind::toggle
        : BackendAgentNativeTimerModifyKind::update;
    request.operationId = op.operationId;
    request.operationRevision = op.operationRevision;
    request.timerAssignmentId = payload.timerAssignmentId;
    request.expectedAssignmentRevision = payload.expectedAssignmentRevision;
    request.expectedIntentRevision = payload.expectedIntentRevision;
    request.assignmentEpoch = payload.assignmentEpoch;
    request.nativeTimerBindingId = payload.nativeTimerBindingId;
    request.expectedBindingRevision = payload.expectedBindingRevision;
    request.backendId = payload.backendId;
    request.backendGeneration = payload.backendGeneration;
    request.backendNativeTimerId = payload.backendNativeTimerId;
    request.expectedCurrentFingerprint = payload.expectedCurrentFingerprint;
    request.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(specification);
    request.specification = specification;
    request.controlPlaneClaimedAt = assignedAt;

    // Persist exactly one Agent assignment before entering dispatching.
    const auto deliver = assigner.assign(
        systemContext(), request, now, now + 300);
    if (!deliver.accepted || deliver.assignment.assignedAt != assignedAt)
        return false;
    NativeTimerModifyDispatchClaimRequest claim;
    claim.operationId = op.operationId;
    claim.expectedOperationRevision = op.operationRevision;
    return dispatcher.claim(claim, assignedAt).ok();
}

void advanceFamily(
    const std::string& family,
    NativeTimerModifyProductiveSummary& summary,
    MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    TimerIntentRepository& intents,
    TimerAssignmentRepository& assignments,
    NativeTimerBindingRepository& bindings,
    BackendAgentNativeTimerModifyAssignmentService& assigner,
    NativeTimerModifyDispatchService& dispatcher,
    NativeTimerModifyReadbackVerificationService& verifier,
    NativeTimerModifyOperationCompletionService& completion,
    const NativeModifyReadbackAcquirer& readback,
    std::int64_t now)
{
    const auto candidates = operations.listByActionFamilyAndStates(
        family,
        {MutationOperationState::accepted,
         MutationOperationState::dispatching,
         MutationOperationState::executedUnverified,
         MutationOperationState::outcomeUnknown}, 32);
    if (!candidates.ok()) return;
    summary.discovered += candidates.operations.size();

    for (const auto& op : candidates.operations)
    {
        if (op.state == MutationOperationState::accepted)
        {
            if (!assignAndClaim(op, operations, commands, agents, intents,
                assignments, bindings, assigner, dispatcher, now))
            {
                ++summary.deferred;
                continue;
            }
            ++summary.claimed;
        }
        const char* type = family == "timer.toggle"
            ? kBackendAgentNativeTimerToggleCommandType
            : kBackendAgentNativeTimerUpdateCommandType;
        const auto assignment = commands.findAssignmentForOperation(
            op.backendId, op.operationId, type);
        if (!assignment || !commands.resultForCommand(assignment->commandId))
        {
            ++summary.deferred;
            continue;
        }
        const auto applied = applyDurableNativeTimerModifyResult(
            commands, operations, bindings, dispatcher,
            assignment->commandId);
        if (!applied.ok())
        {
            ++summary.deferred;
            continue;
        }
        if (applied.status == NativeTimerModifyResultStatus::applied)
            ++summary.applied;
        if (!applied.outcome.expectationPresent) continue;

        const auto observation = readback(applied.outcome.expectation);
        if (!observation)
        {
            ++summary.deferred;
            continue;
        }
        const auto verified =
            verifier.verify(applied.outcome.expectation, *observation);
        if (!verified.ok())
        {
            ++summary.deferred;
            continue;
        }
        const auto completed =
            completion.complete(applied.outcome.expectation, now);
        if (completed.ok())
        {
            if (completed.status ==
                NativeTimerModifyOperationCompletionStatus::completed)
                ++summary.completed;
        }
        else ++summary.deferred;
    }
}
}

NativeTimerModifyProductiveSummary advanceNativeTimerModifyRuntimeOnce(
    MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    TimerIntentRepository& intents,
    TimerAssignmentRepository& assignments,
    NativeTimerBindingRepository& bindings,
    BackendAgentNativeTimerModifyAssignmentService& assigner,
    NativeTimerModifyDispatchService& dispatcher,
    NativeTimerModifyReadbackVerificationService& verifier,
    NativeTimerModifyOperationCompletionService& completion,
    const NativeModifyReadbackAcquirer& readback,
    std::int64_t now)
{
    NativeTimerModifyProductiveSummary summary;
    if (now <= 0 || !readback) return summary;
    advanceFamily("timer.update", summary, operations, commands, agents,
        intents, assignments, bindings, assigner, dispatcher, verifier,
        completion, readback, now);
    advanceFamily("timer.toggle", summary, operations, commands, agents,
        intents, assignments, bindings, assigner, dispatcher, verifier,
        completion, readback, now);
    return summary;
}
}
