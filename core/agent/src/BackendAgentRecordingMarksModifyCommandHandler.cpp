#include "BackendAgentRecordingMarksModifyCommandHandler.h"

#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentRecordingMarksModifyLocalState.h"

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

struct RecordingMarksModifyGenericProjection
{
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string diagnostics;
};

bool recordingMarksModifyGenericProjection(
    vdrsuite::agent::BackendAgentRecordingMarksModifyOutcomeCategory outcome,
    RecordingMarksModifyGenericProjection& projection)
{
    using namespace vdrsuite::agent;
    switch (outcome)
    {
        case BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect:
            projection = {
                "not_started", "verified", "rejected", "fenced", "none",
                "recording marks modify rejected without effect"};
            return true;
        case BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified:
            projection = {
                "accepted_by_executor", "outcome_unknown", "outcome_unknown",
                "none", "reconcile_only",
                "recording marks modify accepted; readback reconciliation required"};
            return true;
        case BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown:
            projection = {
                "starting", "outcome_unknown", "outcome_unknown",
                "executor_unknown", "reconcile_only",
                "recording marks modify outcome unknown; reconciliation required"};
            return true;
    }
    return false;
}

void createResult(
    LocalState& state,
    const RecordingMarksModifyGenericProjection& projection,
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
    const RecordingMarksModifyGenericProjection& projection,
    const vdrsuite::agent::BackendAgentRecordingMarksModifyEvidence& evidence)
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

bool backendAgentRecordingMarksModifyCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentRecordingMarksModifyCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode)
{
    if (!state.stateExtensionPresent ||
        state.stateExtension.extensionType !=
            kBackendAgentRecordingMarksModifyLocalStateExtensionType)
    {
        reasonCode = "recording_marks_modify_local_state_required";
        return false;
    }

    BackendAgentRecordingMarksModifyLocalState localState;
    if (!backendAgentRecordingMarksModifyParseLocalState(
            state.stateExtension.payload, localState, reasonCode))
    {
        reasonCode = "recording_marks_modify_local_state_invalid";
        return false;
    }

    const auto recovery = backendAgentRecordingMarksModifyRecoverLocalState(
        localState,
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration,
        nowSeconds());
    if (recovery.decision ==
        BackendAgentRecordingMarksModifyRecoveryDecision::failClosed)
    {
        reasonCode = recovery.reasonCode.empty()
            ? "recording_marks_modify_recovery_failed"
            : recovery.reasonCode;
        return false;
    }

    bool stateChanged = false;
    if (recovery.decision ==
        BackendAgentRecordingMarksModifyRecoveryDecision::reconcileOnly)
    {
        if (!backendAgentRecordingMarksModifyCompleteLocalState(
                localState, recovery.evidence, reasonCode))
            return false;
        const std::string payload =
            backendAgentRecordingMarksModifySerializeLocalState(
                localState, reasonCode);
        if (payload.empty()) return false;
        state.stateExtension.payload = payload;
        stateChanged = true;
    }
    else if (recovery.decision !=
        BackendAgentRecordingMarksModifyRecoveryDecision::returnPersistedEvidence)
    {
        reasonCode = "recording_marks_modify_recovery_decision_invalid";
        return false;
    }

    RecordingMarksModifyGenericProjection projection;
    if (!recordingMarksModifyGenericProjection(
            recovery.evidence.outcome, projection))
    {
        reasonCode = "recording_marks_modify_outcome_projection_invalid";
        return false;
    }

    if (state.resultPresent)
    {
        if (!genericResultMatchesEvidence(
                state, projection, recovery.evidence))
        {
            reasonCode = "recording_marks_modify_result_evidence_conflict";
            return false;
        }
    }
    else
    {
        createResult(state, projection, recovery.evidence.completedAt);
        stateChanged = true;
    }

    if (stateChanged)
        return persist(statePath, state, reasonCode);

    reasonCode = "recording_marks_modify_local_state_reconciled";
    return true;
}

bool backendAgentRecordingMarksModifyCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode)
{
    if (state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "not_started" || currentTime <= 0 ||
        currentTime > state.assignment.deadline)
    {
        reasonCode = "recording_marks_modify_fresh_starting_state_invalid";
        return false;
    }

    BackendAgentRecordingMarksModifyLocalState localState;
    if (!backendAgentRecordingMarksModifyPrepareLocalStarting(
            state.assignment, currentTime, localState, reasonCode))
        return false;

    BackendAgentCommandStateExtension extension;
    extension.extensionType =
        kBackendAgentRecordingMarksModifyLocalStateExtensionType;
    extension.commandId = state.assignment.commandId;
    extension.requestFingerprint = state.assignment.requestFingerprint;
    extension.payload = backendAgentRecordingMarksModifySerializeLocalState(
        localState, reasonCode);
    if (extension.payload.empty() ||
        !backendAgentCommandStateExtensionValidateSupported(
            extension, state.assignment, reasonCode))
    {
        reasonCode = "recording_marks_modify_fresh_starting_extension_invalid";
        return false;
    }

    state.stateExtensionPresent = true;
    state.stateExtension = extension;
    state.dispatchState = "starting";
    if (!persist(statePath, state, reasonCode)) return false;
    reasonCode = "recording_marks_modify_local_starting_persisted";
    return true;
}

bool backendAgentRecordingMarksModifyCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentRecordingMarksModifyCommandContext& context,
    IBackendAgentRecordingMarksModifyTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode)
{
    if (transport == nullptr ||
        !state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "starting" || !state.receiptAcknowledged ||
        state.stateExtension.extensionType !=
            kBackendAgentRecordingMarksModifyLocalStateExtensionType)
    {
        reasonCode = "recording_marks_modify_executor_handoff_state_invalid";
        return false;
    }

    BackendAgentRecordingMarksModifyLocalState localState;
    if (!backendAgentRecordingMarksModifyParseLocalState(
            state.stateExtension.payload, localState, reasonCode) ||
        localState.phase != BackendAgentRecordingMarksModifyLocalPhase::starting)
    {
        reasonCode = "recording_marks_modify_executor_starting_state_invalid";
        return false;
    }

    BackendAgentRecordingMarksModifyExecutorContext executorContext;
    executorContext.backendId = context.backendId;
    executorContext.agentId = context.agentId;
    executorContext.agentInstanceId = context.agentInstanceId;
    executorContext.backendGeneration = context.backendGeneration;
    executorContext.now = nowSeconds();

    BackendAgentRecordingMarksModifyEvidence evidence;
    if (!backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
            state.assignment,
            localState,
            executorContext,
            *transport,
            evidence,
            reasonCode))
        return false;

    if (!backendAgentRecordingMarksModifyCompleteLocalState(
            localState, evidence, reasonCode))
        return false;
    const std::string payload =
        backendAgentRecordingMarksModifySerializeLocalState(
            localState, reasonCode);
    if (payload.empty()) return false;
    state.stateExtension.payload = payload;

    RecordingMarksModifyGenericProjection projection;
    if (!recordingMarksModifyGenericProjection(evidence.outcome, projection))
    {
        reasonCode = "recording_marks_modify_executor_outcome_projection_invalid";
        return false;
    }
    createResult(state, projection, evidence.completedAt);

    if (!persist(statePath, state, reasonCode)) return false;
    reasonCode = "recording_marks_modify_executor_outcome_persisted";
    return true;
}

}
