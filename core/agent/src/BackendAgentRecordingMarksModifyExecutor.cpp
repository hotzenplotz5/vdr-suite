#include "BackendAgentRecordingMarksModifyExecutor.h"

#include <algorithm>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

bool sameCommand(
    const BackendAgentRecordingMarksModifyCommand& left,
    const BackendAgentRecordingMarksModifyCommand& right)
{
    return left.kind == right.kind &&
        left.commandId == right.commandId &&
        left.requestFingerprint == right.requestFingerprint &&
        left.operationId == right.operationId &&
        left.operationRevision == right.operationRevision &&
        left.recordingKey == right.recordingKey &&
        left.expectedMarksRevision == right.expectedMarksRevision &&
        left.sourceFrame == right.sourceFrame &&
        left.targetFrame == right.targetFrame &&
        left.replacementFrames == right.replacementFrames &&
        left.jobId == right.jobId &&
        left.attemptId == right.attemptId &&
        left.claimEpoch == right.claimEpoch &&
        left.backendId == right.backendId &&
        left.agentId == right.agentId &&
        left.agentInstanceId == right.agentInstanceId &&
        left.backendGeneration == right.backendGeneration &&
        left.controlPlaneClaimedAt == right.controlPlaneClaimedAt &&
        backendAgentLocalProviderSameFence(
            left.localProviderSelection, right.localProviderSelection);
}

bool sameContext(
    const BackendAgentRecordingMarksModifyCommand& command,
    const BackendAgentRecordingMarksModifyExecutorContext& context)
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

BackendAgentRecordingMarksModifyEvidence evidenceFor(
    const BackendAgentRecordingMarksModifyCommand& command,
    std::int64_t localStartingPersistedAt,
    BackendAgentRecordingMarksModifyOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    std::string evidenceReference)
{
    BackendAgentRecordingMarksModifyEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.jobId = command.jobId;
    evidence.attemptId = command.attemptId;
    evidence.claimEpoch = command.claimEpoch;
    evidence.backendId = command.backendId;
    evidence.agentId = command.agentId;
    evidence.agentInstanceId = command.agentInstanceId;
    evidence.backendGeneration = command.backendGeneration;
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = localStartingPersistedAt;
    evidence.outcome = outcome;
    evidence.dispatchStartedAt = dispatchStartedAt;
    evidence.completedAt = completedAt;
    evidence.evidenceReference = std::move(evidenceReference);
    return evidence;
}

bool finish(
    const BackendAgentRecordingMarksModifyLocalState& localState,
    BackendAgentRecordingMarksModifyOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    const std::string& reference,
    const std::string& reason,
    BackendAgentRecordingMarksModifyEvidence& evidence,
    std::string& reasonCode)
{
    auto candidate = evidenceFor(
        localState.command,
        localState.localStartingPersistedAt,
        outcome,
        dispatchStartedAt,
        completedAt,
        reference);
    std::string validationReason;
    if (!backendAgentRecordingMarksModifyEvidenceMatches(
            candidate, localState.command, validationReason))
    {
        reasonCode = "recording_marks_modify_executor_evidence_invalid";
        return false;
    }
    evidence = std::move(candidate);
    reasonCode = reason;
    return true;
}

bool rejectWithoutEffect(
    const BackendAgentRecordingMarksModifyLocalState& localState,
    std::int64_t now,
    const std::string& reference,
    BackendAgentRecordingMarksModifyEvidence& evidence,
    std::string& reasonCode)
{
    return finish(
        localState,
        BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect,
        0,
        std::max(now, localState.localStartingPersistedAt),
        reference,
        "recording_marks_modify_executor_rejected_without_effect",
        evidence,
        reasonCode);
}

bool safeEvidenceReference(const std::string& value)
{
    return !value.empty() && backendAgentCommandSafeText(value, 512);
}

}

bool backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentRecordingMarksModifyLocalState& localState,
    const BackendAgentRecordingMarksModifyExecutorContext& context,
    IBackendAgentRecordingMarksModifyTransport& transport,
    BackendAgentRecordingMarksModifyEvidence& evidence,
    std::string& reasonCode)
{
    evidence = {};
    std::string validationReason;
    BackendAgentRecordingMarksModifyCommand expectedCommand;
    if (!backendAgentRecordingMarksModifyLocalStateValid(
            localState, validationReason) ||
        localState.phase != BackendAgentRecordingMarksModifyLocalPhase::starting ||
        !backendAgentRecordingMarksModifyCommandFromAssignment(
            assignment, expectedCommand, validationReason) ||
        !sameCommand(expectedCommand, localState.command) ||
        context.now <= 0 ||
        context.now < localState.localStartingPersistedAt)
    {
        reasonCode = "recording_marks_modify_executor_fresh_state_invalid";
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

    BackendAgentRecordingMarksModifyTransportRequest request;
    request.command = localState.command;
    request.localStartingPersistedAt = localState.localStartingPersistedAt;

    const std::int64_t dispatchStartedAt = context.now;
    BackendAgentRecordingMarksModifyTransportReply reply;
    try
    {
        reply = transport.modifyMarks(request);
    }
    catch (...)
    {
        return finish(
            localState,
            BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown,
            dispatchStartedAt,
            context.now,
            "executor:dispatch-exception",
            "recording_marks_modify_executor_outcome_unknown",
            evidence,
            reasonCode);
    }

    switch (reply.disposition)
    {
        case BackendAgentRecordingMarksModifyTransportDisposition::rejectedWithoutEffect:
            return rejectWithoutEffect(
                localState,
                context.now,
                safeEvidenceReference(reply.evidenceReference)
                    ? reply.evidenceReference
                    : "executor:transport-rejected",
                evidence,
                reasonCode);

        case BackendAgentRecordingMarksModifyTransportDisposition::acceptedUnverified:
            if (!safeEvidenceReference(reply.evidenceReference))
            {
                return finish(
                    localState,
                    BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown,
                    dispatchStartedAt,
                    context.now,
                    "executor:accepted-evidence-invalid",
                    "recording_marks_modify_executor_outcome_unknown",
                    evidence,
                    reasonCode);
            }
            return finish(
                localState,
                BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified,
                dispatchStartedAt,
                context.now,
                reply.evidenceReference,
                "recording_marks_modify_executor_accepted_unverified",
                evidence,
                reasonCode);

        case BackendAgentRecordingMarksModifyTransportDisposition::outcomeUnknown:
            return finish(
                localState,
                BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown,
                dispatchStartedAt,
                context.now,
                safeEvidenceReference(reply.evidenceReference)
                    ? reply.evidenceReference
                    : "executor:transport-outcome-unknown",
                "recording_marks_modify_executor_outcome_unknown",
                evidence,
                reasonCode);
    }

    return finish(
        localState,
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown,
        dispatchStartedAt,
        context.now,
        "executor:transport-disposition-invalid",
        "recording_marks_modify_executor_outcome_unknown",
        evidence,
        reasonCode);
}

}
