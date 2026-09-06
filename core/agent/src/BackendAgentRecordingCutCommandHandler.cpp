#include "BackendAgentRecordingCutCommandHandler.h"

#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentRecordingCutLocalState.h"

#include <chrono>
#include <string>

namespace
{
using vdrsuite::agent::commandstate::LocalState;
using vdrsuite::agent::commandstate::persist;

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

struct RecordingCutGenericProjection
{
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string diagnostics;
};

bool recordingCutGenericProjection(
    vdrsuite::agent::BackendAgentRecordingCutOutcomeCategory outcome,
    RecordingCutGenericProjection& projection)
{
    using namespace vdrsuite::agent;
    switch (outcome)
    {
        case BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect:
            projection = {
                "not_started", "verified", "rejected", "fenced", "none",
                "recording cut rejected without effect"};
            return true;
        case BackendAgentRecordingCutOutcomeCategory::acceptedUnverified:
            projection = {
                "accepted_by_executor", "outcome_unknown", "outcome_unknown",
                "none", "reconcile_only",
                "recording cut accepted; native result reconciliation required"};
            return true;
        case BackendAgentRecordingCutOutcomeCategory::outcomeUnknown:
            projection = {
                "starting", "outcome_unknown", "outcome_unknown",
                "executor_unknown", "reconcile_only",
                "recording cut outcome unknown; reconciliation required"};
            return true;
    }
    return false;
}

bool bindAcceptedEvidence(
    const vdrsuite::agent::BackendAgentRecordingCutEvidence& evidence,
    RecordingCutGenericProjection& projection)
{
    using namespace vdrsuite::agent;
    if (evidence.outcome != BackendAgentRecordingCutOutcomeCategory::acceptedUnverified)
        return true;
    if (!backendAgentCommandSafeText(evidence.evidenceReference, 512)) return false;
    projection.diagnostics += "; evidence=" + evidence.evidenceReference;
    return backendAgentCommandSafeText(projection.diagnostics, 1024);
}

void createResult(
    LocalState& state,
    const RecordingCutGenericProjection& projection,
    std::int64_t completedAt)
{
    state.dispatchState = projection.dispatchState;
    state.resultPresent = true;
    state.resultAcknowledged = false;
    auto& result = state.result;
    const auto& assignment = state.assignment;
    result.commandId = assignment.commandId;
    result.requestFingerprint = assignment.requestFingerprint;
    result.jobId = assignment.jobId;
    result.attemptId = assignment.attemptId;
    result.claimEpoch = assignment.claimEpoch;
    result.backendId = assignment.backendId;
    result.agentId = assignment.agentId;
    result.agentInstanceId = assignment.agentInstanceId;
    result.backendGeneration = assignment.backendGeneration;
    result.dispatchState = projection.dispatchState;
    result.verificationState = projection.verificationState;
    result.resultCategory = projection.resultCategory;
    result.errorCategory = projection.errorCategory;
    result.retryClassification = projection.retryClassification;
    result.boundedDiagnostics = projection.diagnostics;
    result.completedAt = completedAt;
}

bool genericResultMatchesEvidence(
    const LocalState& state,
    const RecordingCutGenericProjection& projection,
    const vdrsuite::agent::BackendAgentRecordingCutEvidence& evidence)
{
    if (!state.resultPresent) return false;
    const auto& result = state.result;
    return state.dispatchState == projection.dispatchState &&
        result.dispatchState == projection.dispatchState &&
        result.verificationState == projection.verificationState &&
        result.resultCategory == projection.resultCategory &&
        result.errorCategory == projection.errorCategory &&
        result.retryClassification == projection.retryClassification &&
        result.boundedDiagnostics == projection.diagnostics &&
        result.completedAt == evidence.completedAt;
}

}

namespace vdrsuite::agent
{

bool backendAgentRecordingCutCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentRecordingCutCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode)
{
    if (!state.stateExtensionPresent ||
        state.stateExtension.extensionType !=
            kBackendAgentRecordingCutLocalStateExtensionType)
    {
        reasonCode = "recording_cut_local_state_required";
        return false;
    }

    BackendAgentRecordingCutLocalState localState;
    if (!backendAgentRecordingCutParseLocalState(
            state.stateExtension.payload, localState, reasonCode))
    {
        reasonCode = "recording_cut_local_state_invalid";
        return false;
    }

    const auto recovery = backendAgentRecordingCutRecoverLocalState(
        localState,
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration,
        nowSeconds());
    if (recovery.decision == BackendAgentRecordingCutRecoveryDecision::failClosed)
    {
        reasonCode = recovery.reasonCode.empty()
            ? "recording_cut_recovery_failed"
            : recovery.reasonCode;
        return false;
    }

    bool stateChanged = false;
    if (recovery.decision == BackendAgentRecordingCutRecoveryDecision::reconcileOnly)
    {
        if (!backendAgentRecordingCutCompleteLocalState(
                localState, recovery.evidence, reasonCode)) return false;
        const std::string payload = backendAgentRecordingCutSerializeLocalState(
            localState, reasonCode);
        if (payload.empty()) return false;
        state.stateExtension.payload = payload;
        stateChanged = true;
    }
    else if (recovery.decision !=
        BackendAgentRecordingCutRecoveryDecision::returnPersistedEvidence)
    {
        reasonCode = "recording_cut_recovery_decision_invalid";
        return false;
    }

    RecordingCutGenericProjection projection;
    if (!recordingCutGenericProjection(recovery.evidence.outcome, projection) ||
        !bindAcceptedEvidence(recovery.evidence, projection))
    {
        reasonCode = "recording_cut_outcome_projection_invalid";
        return false;
    }

    if (state.resultPresent)
    {
        if (!genericResultMatchesEvidence(state, projection, recovery.evidence))
        {
            reasonCode = "recording_cut_result_evidence_conflict";
            return false;
        }
    }
    else
    {
        createResult(state, projection, recovery.evidence.completedAt);
        stateChanged = true;
    }

    if (stateChanged) return persist(statePath, state, reasonCode);

    reasonCode = "recording_cut_local_state_reconciled";
    return true;
}

bool backendAgentRecordingCutCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode)
{
    if (state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "not_started" || currentTime <= 0 ||
        currentTime > state.assignment.deadline)
    {
        reasonCode = "recording_cut_fresh_starting_state_invalid";
        return false;
    }

    BackendAgentRecordingCutLocalState localState;
    if (!backendAgentRecordingCutPrepareLocalStarting(
            state.assignment, currentTime, localState, reasonCode)) return false;

    BackendAgentCommandStateExtension extension;
    extension.extensionType = kBackendAgentRecordingCutLocalStateExtensionType;
    extension.commandId = state.assignment.commandId;
    extension.requestFingerprint = state.assignment.requestFingerprint;
    extension.payload = backendAgentRecordingCutSerializeLocalState(
        localState, reasonCode);
    if (extension.payload.empty() ||
        !backendAgentCommandStateExtensionValidateSupported(
            extension, state.assignment, reasonCode))
    {
        reasonCode = "recording_cut_fresh_starting_extension_invalid";
        return false;
    }

    state.stateExtensionPresent = true;
    state.stateExtension = extension;
    state.dispatchState = "starting";
    if (!persist(statePath, state, reasonCode)) return false;
    reasonCode = "recording_cut_local_starting_persisted";
    return true;
}

bool backendAgentRecordingCutCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentRecordingCutCommandContext& context,
    IBackendAgentRecordingCutTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode)
{
    if (transport == nullptr || !state.stateExtensionPresent ||
        state.resultPresent || state.dispatchState != "starting" ||
        !state.receiptAcknowledged ||
        state.stateExtension.extensionType != kBackendAgentRecordingCutLocalStateExtensionType)
    {
        reasonCode = "recording_cut_executor_handoff_state_invalid";
        return false;
    }

    BackendAgentRecordingCutLocalState localState;
    if (!backendAgentRecordingCutParseLocalState(
            state.stateExtension.payload, localState, reasonCode) ||
        localState.phase != BackendAgentRecordingCutLocalPhase::starting)
    {
        reasonCode = "recording_cut_executor_starting_state_invalid";
        return false;
    }

    BackendAgentRecordingCutExecutorContext executorContext;
    executorContext.backendId = context.backendId;
    executorContext.agentId = context.agentId;
    executorContext.agentInstanceId = context.agentInstanceId;
    executorContext.backendGeneration = context.backendGeneration;
    executorContext.now = nowSeconds();

    BackendAgentRecordingCutEvidence evidence;
    if (!backendAgentRecordingCutExecuteFreshStartingOnce(
            state.assignment, localState, executorContext, *transport,
            evidence, reasonCode)) return false;

    if (!backendAgentRecordingCutCompleteLocalState(
            localState, evidence, reasonCode)) return false;
    const std::string payload = backendAgentRecordingCutSerializeLocalState(
        localState, reasonCode);
    if (payload.empty()) return false;
    state.stateExtension.payload = payload;

    RecordingCutGenericProjection projection;
    if (!recordingCutGenericProjection(evidence.outcome, projection) ||
        !bindAcceptedEvidence(evidence, projection))
    {
        reasonCode = "recording_cut_executor_outcome_projection_invalid";
        return false;
    }
    createResult(state, projection, evidence.completedAt);

    if (!persist(statePath, state, reasonCode)) return false;
    reasonCode = "recording_cut_executor_outcome_persisted";
    return true;
}

}
