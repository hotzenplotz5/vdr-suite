#include "BackendAgentNativeTimerDeleteCommandHandler.h"

#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"

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

struct NativeTimerDeleteGenericProjection
{
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string diagnostics;
};

bool nativeTimerDeleteGenericProjection(
    vdrsuite::agent::BackendAgentNativeTimerDeleteOutcomeCategory outcome,
    NativeTimerDeleteGenericProjection& projection)
{
    using namespace vdrsuite::agent;
    switch (outcome)
    {
        case BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect:
            projection = {
                "not_started", "verified", "rejected", "fenced", "none",
                "native Timer delete rejected without effect"};
            return true;
        case BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified:
            projection = {
                "accepted_by_executor", "outcome_unknown", "outcome_unknown",
                "none", "reconcile_only",
                "native Timer delete accepted; readback reconciliation required"};
            return true;
        case BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown:
            projection = {
                "starting", "outcome_unknown", "outcome_unknown",
                "executor_unknown", "reconcile_only",
                "native Timer delete outcome unknown; reconciliation required"};
            return true;
    }
    return false;
}

void createTimerDeleteResult(
    LocalState& state,
    const NativeTimerDeleteGenericProjection& projection,
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

bool genericResultMatchesTimerDeleteEvidence(
    const LocalState& state,
    const NativeTimerDeleteGenericProjection& projection,
    const vdrsuite::agent::BackendAgentNativeTimerDeleteEvidence& evidence)
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

bool backendAgentNativeTimerDeleteCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerDeleteCommandContext& context,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (!state.stateExtensionPresent ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerDeleteLocalStateExtensionType)
    {
        reason = "native_delete_local_state_required";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState localState;
    if (!backendAgentNativeTimerDeleteParseLocalState(
            state.stateExtension.payload, localState, reason))
    {
        reason = "native_delete_local_state_invalid";
        return false;
    }

    const auto recovery = backendAgentNativeTimerDeleteRecoverLocalState(
        localState,
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration,
        nowSeconds());
    if (recovery.decision ==
        BackendAgentNativeTimerDeleteRecoveryDecision::failClosed)
    {
        reason = recovery.reasonCode.empty()
            ? "native_delete_recovery_failed" : recovery.reasonCode;
        return false;
    }

    bool stateChanged = false;
    if (recovery.decision ==
        BackendAgentNativeTimerDeleteRecoveryDecision::reconcileOnly)
    {
        if (!backendAgentNativeTimerDeleteCompleteLocalState(
                localState, recovery.evidence, reason))
            return false;
        const std::string payload =
            backendAgentNativeTimerDeleteSerializeLocalState(
                localState, reason);
        if (payload.empty()) return false;
        state.stateExtension.payload = payload;
        stateChanged = true;
    }
    else if (recovery.decision !=
        BackendAgentNativeTimerDeleteRecoveryDecision::returnPersistedEvidence)
    {
        reason = "native_delete_recovery_decision_invalid";
        return false;
    }

    NativeTimerDeleteGenericProjection projection;
    if (!nativeTimerDeleteGenericProjection(
            recovery.evidence.outcome, projection))
    {
        reason = "native_delete_outcome_projection_invalid";
        return false;
    }

    if (state.resultPresent)
    {
        if (!genericResultMatchesTimerDeleteEvidence(
                state, projection, recovery.evidence))
        {
            reason = "native_delete_result_evidence_conflict";
            return false;
        }
    }
    else
    {
        createTimerDeleteResult(state, projection, recovery.evidence.completedAt);
        stateChanged = true;
    }

    if (stateChanged)
        return persist(statePath, state, reason);

    reason = "native_delete_local_state_reconciled";
    return true;
}

bool backendAgentNativeTimerDeleteCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    if (state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "not_started" || currentTime <= 0 ||
        currentTime > state.assignment.deadline)
    {
        reason = "native_delete_fresh_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState localState;
    if (!backendAgentNativeTimerDeletePrepareLocalStarting(
            state.assignment, currentTime, localState, reason))
        return false;

    BackendAgentCommandStateExtension extension;
    extension.extensionType =
        kBackendAgentNativeTimerDeleteLocalStateExtensionType;
    extension.commandId = state.assignment.commandId;
    extension.requestFingerprint = state.assignment.requestFingerprint;
    extension.payload = backendAgentNativeTimerDeleteSerializeLocalState(
        localState, reason);
    if (extension.payload.empty() ||
        !backendAgentCommandStateExtensionValidateSupported(
            extension, state.assignment, reason))
    {
        reason = "native_delete_fresh_starting_extension_invalid";
        return false;
    }

    state.stateExtensionPresent = true;
    state.stateExtension = extension;
    state.dispatchState = "starting";
    if (!persist(statePath, state, reason)) return false;
    reason = "native_delete_local_starting_persisted";
    return true;
}

bool backendAgentNativeTimerDeleteCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerDeleteCommandContext& context,
    IBackendAgentNativeTimerDeleteTransport* transport,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (transport == nullptr ||
        !state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "starting" || !state.receiptAcknowledged ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerDeleteLocalStateExtensionType)
    {
        reason = "native_delete_executor_handoff_state_invalid";
        return false;
    }

    BackendAgentNativeTimerDeleteLocalState localState;
    if (!backendAgentNativeTimerDeleteParseLocalState(
            state.stateExtension.payload, localState, reason) ||
        localState.phase != BackendAgentNativeTimerDeleteLocalPhase::starting)
    {
        reason = "native_delete_executor_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerDeleteExecutorContext executorContext;
    executorContext.backendId = context.backendId;
    executorContext.agentId = context.agentId;
    executorContext.agentInstanceId = context.agentInstanceId;
    executorContext.backendGeneration = context.backendGeneration;
    executorContext.now = nowSeconds();

    BackendAgentNativeTimerDeleteEvidence evidence;
    if (!backendAgentNativeTimerDeleteExecuteFreshStartingOnce(
            state.assignment,
            localState,
            executorContext,
            *transport,
            evidence,
            reason))
        return false;

    if (!backendAgentNativeTimerDeleteCompleteLocalState(
            localState, evidence, reason))
        return false;
    const std::string payload = backendAgentNativeTimerDeleteSerializeLocalState(
        localState, reason);
    if (payload.empty()) return false;
    state.stateExtension.payload = payload;

    NativeTimerDeleteGenericProjection projection;
    if (!nativeTimerDeleteGenericProjection(evidence.outcome, projection))
    {
        reason = "native_delete_executor_outcome_projection_invalid";
        return false;
    }
    createTimerDeleteResult(state, projection, evidence.completedAt);

    if (!persist(statePath, state, reason)) return false;
    reason = "native_delete_executor_outcome_persisted";
    return true;
}

}
