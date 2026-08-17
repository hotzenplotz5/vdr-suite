#include "BackendAgentNativeTimerCreateExecutor.h"

#include <algorithm>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

bool sameSpecification(
    const BackendAgentNativeTimerCreateSpecification& left,
    const BackendAgentNativeTimerCreateSpecification& right)
{
    return left.channelId == right.channelId &&
        left.title == right.title &&
        left.directory == right.directory &&
        left.day == right.day &&
        left.weekdays == right.weekdays &&
        left.startTime == right.startTime &&
        left.endTime == right.endTime &&
        left.priority == right.priority &&
        left.lifetime == right.lifetime &&
        left.enabled == right.enabled &&
        left.vps == right.vps;
}

bool sameCommand(
    const BackendAgentNativeTimerCreateCommand& left,
    const BackendAgentNativeTimerCreateCommand& right)
{
    return left.commandId == right.commandId &&
        left.requestFingerprint == right.requestFingerprint &&
        left.operationId == right.operationId &&
        left.operationRevision == right.operationRevision &&
        left.timerAssignmentId == right.timerAssignmentId &&
        left.expectedAssignmentRevision == right.expectedAssignmentRevision &&
        left.expectedIntentRevision == right.expectedIntentRevision &&
        left.assignmentEpoch == right.assignmentEpoch &&
        left.nativeTimerBindingId == right.nativeTimerBindingId &&
        left.expectedSpecificationFingerprint ==
            right.expectedSpecificationFingerprint &&
        left.jobId == right.jobId &&
        left.attemptId == right.attemptId &&
        left.claimEpoch == right.claimEpoch &&
        left.backendId == right.backendId &&
        left.agentId == right.agentId &&
        left.agentInstanceId == right.agentInstanceId &&
        left.backendGeneration == right.backendGeneration &&
        left.controlPlaneClaimedAt == right.controlPlaneClaimedAt &&
        sameSpecification(left.specification, right.specification) &&
        backendAgentLocalProviderSameFence(
            left.localProviderSelection, right.localProviderSelection);
}

bool sameContext(
    const BackendAgentNativeTimerCreateCommand& command,
    const BackendAgentNativeTimerCreateExecutorContext& context)
{
    return command.backendId == context.backendId &&
        command.agentId == context.agentId &&
        command.agentInstanceId == context.agentInstanceId &&
        command.backendGeneration == context.backendGeneration;
}

bool selectedProviderStillCurrent(
    const BackendAgentLocalProviderSelection& selection,
    const BackendAgentLocalProviderFacts& facts)
{
    return backendAgentLocalProviderValidSelection(selection) &&
        backendAgentLocalProviderValidFacts(facts) &&
        facts.available &&
        selection.providerId == facts.providerId &&
        selection.providerKind == facts.providerKind &&
        selection.providerInstanceEpoch == facts.providerInstanceEpoch &&
        selection.providerGeneration == facts.providerGeneration &&
        selection.capabilityRevision == facts.capabilityRevision &&
        std::find(
            facts.capabilities.begin(),
            facts.capabilities.end(),
            selection.requiredCapability) != facts.capabilities.end();
}

BackendAgentNativeTimerCreateEvidence evidenceFor(
    const BackendAgentNativeTimerCreateCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentNativeTimerCreateOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    std::string evidenceReference)
{
    BackendAgentNativeTimerCreateEvidence value;
    value.commandId = command.commandId;
    value.requestFingerprint = command.requestFingerprint;
    value.operationId = command.operationId;
    value.operationRevision = command.operationRevision;
    value.timerAssignmentId = command.timerAssignmentId;
    value.nativeTimerBindingId = command.nativeTimerBindingId;
    value.jobId = command.jobId;
    value.attemptId = command.attemptId;
    value.claimEpoch = command.claimEpoch;
    value.backendId = command.backendId;
    value.agentId = command.agentId;
    value.agentInstanceId = command.agentInstanceId;
    value.backendGeneration = command.backendGeneration;
    value.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    value.localStartingPersistedAt = localStartingPersistedAt;
    value.outcome = outcome;
    value.dispatchStartedAt = dispatchStartedAt;
    value.completedAt = completedAt;
    value.evidenceReference = std::move(evidenceReference);
    return value;
}

bool finish(
    const BackendAgentNativeTimerCreateLocalState& localState,
    BackendAgentNativeTimerCreateOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    const std::string& reference,
    const std::string& reason,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    BackendAgentNativeTimerCreateEvidence candidate = evidenceFor(
        localState.command,
        localState.localStartingPersistedAt,
        outcome,
        dispatchStartedAt,
        completedAt,
        reference);

    std::string validationReason;
    if (!backendAgentNativeTimerCreateEvidenceMatches(
            candidate, localState.command, validationReason))
    {
        reasonCode = "native_timer_create_executor_evidence_invalid";
        return false;
    }

    evidence = std::move(candidate);
    reasonCode = reason;
    return true;
}

bool rejectWithoutEffect(
    const BackendAgentNativeTimerCreateLocalState& localState,
    std::int64_t now,
    const std::string& reference,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    return finish(
        localState,
        BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect,
        0,
        std::max(now, localState.localStartingPersistedAt),
        reference,
        "native_timer_create_executor_rejected_without_effect",
        evidence,
        reasonCode);
}

bool safeEvidenceReference(const std::string& value)
{
    return !value.empty() && ::backendAgentCommandSafeText(value, 512);
}

} // namespace

bool backendAgentNativeTimerCreateExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentNativeTimerCreateLocalState& localState,
    const BackendAgentNativeTimerCreateExecutorContext& context,
    IBackendAgentNativeTimerCreateTransport& transport,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode)
{
    evidence = {};

    std::string validationReason;
    BackendAgentNativeTimerCreateCommand expectedCommand;

    if (!backendAgentNativeTimerCreateLocalStateValid(
            localState, validationReason) ||
        localState.phase != BackendAgentNativeTimerCreateLocalPhase::starting ||
        !backendAgentNativeTimerCreateCommandFromAssignment(
            assignment, expectedCommand, validationReason) ||
        !sameCommand(expectedCommand, localState.command) ||
        context.now <= 0 ||
        context.now < localState.localStartingPersistedAt)
    {
        reasonCode = "native_timer_create_executor_fresh_state_invalid";
        return false;
    }

    if (!sameContext(localState.command, context))
    {
        return rejectWithoutEffect(
            localState,
            context.now,
            "executor-fence:agent-generation",
            evidence,
            reasonCode);
    }

    if (assignment.deadline <= context.now)
    {
        return rejectWithoutEffect(
            localState,
            context.now,
            "executor-fence:deadline",
            evidence,
            reasonCode);
    }

    BackendAgentLocalProviderFacts facts;
    std::string discoveryReason;

    try
    {
        if (!transport.discoverProvider(facts, discoveryReason))
        {
            return rejectWithoutEffect(
                localState,
                context.now,
                "executor-fence:provider-discovery",
                evidence,
                reasonCode);
        }
    }
    catch (...)
    {
        return rejectWithoutEffect(
            localState,
            context.now,
            "executor-fence:provider-discovery-exception",
            evidence,
            reasonCode);
    }

    if (!selectedProviderStillCurrent(
            localState.command.localProviderSelection, facts))
    {
        return rejectWithoutEffect(
            localState,
            context.now,
            "executor-fence:provider-changed",
            evidence,
            reasonCode);
    }

    BackendAgentNativeTimerCreateTransportRequest request;
    request.command = localState.command;
    request.localStartingPersistedAt = localState.localStartingPersistedAt;

    const std::int64_t dispatchStartedAt = context.now;

    BackendAgentNativeTimerCreateTransportReply reply;
    try
    {
        reply = transport.createTimer(request);
    }
    catch (...)
    {
        return finish(
            localState,
            BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown,
            dispatchStartedAt,
            context.now,
            "executor:dispatch-exception",
            "native_timer_create_executor_outcome_unknown",
            evidence,
            reasonCode);
    }

    switch (reply.disposition)
    {
        case BackendAgentNativeTimerCreateTransportDisposition::rejectedWithoutEffect:
            return rejectWithoutEffect(
                localState,
                context.now,
                safeEvidenceReference(reply.evidenceReference)
                    ? reply.evidenceReference
                    : "executor:transport-rejected",
                evidence,
                reasonCode);

        case BackendAgentNativeTimerCreateTransportDisposition::acceptedUnverified:
            if (!safeEvidenceReference(reply.evidenceReference))
            {
                return finish(
                    localState,
                    BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown,
                    dispatchStartedAt,
                    context.now,
                    "executor:accepted-evidence-invalid",
                    "native_timer_create_executor_outcome_unknown",
                    evidence,
                    reasonCode);
            }

            return finish(
                localState,
                BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified,
                dispatchStartedAt,
                context.now,
                reply.evidenceReference,
                "native_timer_create_executor_accepted_unverified",
                evidence,
                reasonCode);

        case BackendAgentNativeTimerCreateTransportDisposition::outcomeUnknown:
            return finish(
                localState,
                BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown,
                dispatchStartedAt,
                context.now,
                safeEvidenceReference(reply.evidenceReference)
                    ? reply.evidenceReference
                    : "executor:transport-outcome-unknown",
                "native_timer_create_executor_outcome_unknown",
                evidence,
                reasonCode);
    }

    return finish(
        localState,
        BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown,
        dispatchStartedAt,
        context.now,
        "executor:transport-disposition-invalid",
        "native_timer_create_executor_outcome_unknown",
        evidence,
        reasonCode);
}

} // namespace vdrsuite::agent
