#include "BackendAgentNativeTimerModifyCommandHandler.h"

#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerModifyLocalState.h"

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

struct NativeTimerModifyGenericProjection
{
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string diagnostics;
};

bool nativeTimerModifyGenericProjection(
    vdrsuite::agent::BackendAgentNativeTimerModifyOutcomeCategory outcome,
    NativeTimerModifyGenericProjection& projection)
{
    using namespace vdrsuite::agent;
    switch (outcome)
    {
        case BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect:
            projection = {
                "not_started", "verified", "rejected", "fenced", "none",
                "native Timer modify rejected without effect"};
            return true;
        case BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified:
            projection = {
                "accepted_by_executor", "outcome_unknown", "outcome_unknown",
                "none", "reconcile_only",
                "native Timer modify accepted; readback reconciliation required"};
            return true;
        case BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown:
            projection = {
                "starting", "outcome_unknown", "outcome_unknown",
                "executor_unknown", "reconcile_only",
                "native Timer modify outcome unknown; reconciliation required"};
            return true;
    }
    return false;
}

void createTimerDeleteResult(
    LocalState& state,
    const NativeTimerModifyGenericProjection& projection,
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
    const NativeTimerModifyGenericProjection& projection,
    const vdrsuite::agent::BackendAgentNativeTimerModifyEvidence& evidence)
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

bool backendAgentNativeTimerModifyCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerModifyCommandContext& context,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (!state.stateExtensionPresent ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerModifyLocalStateExtensionType)
    {
        reason = "native_modify_local_state_required";
        return false;
    }

    BackendAgentNativeTimerModifyLocalState localState;
    if (!backendAgentNativeTimerModifyParseLocalState(
            state.stateExtension.payload, localState, reason))
    {
        reason = "native_modify_local_state_invalid";
        return false;
    }

    const auto recovery = backendAgentNativeTimerModifyRecoverLocalState(
        localState,
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration,
        nowSeconds());
    if (recovery.decision ==
        BackendAgentNativeTimerModifyRecoveryDecision::failClosed)
    {
        reason = recovery.reasonCode.empty()
            ? "native_modify_recovery_failed" : recovery.reasonCode;
        return false;
    }

    bool stateChanged = false;
    if (recovery.decision ==
        BackendAgentNativeTimerModifyRecoveryDecision::reconcileOnly)
    {
        if (!backendAgentNativeTimerModifyCompleteLocalState(
                localState, recovery.evidence, reason))
            return false;
        const std::string payload =
            backendAgentNativeTimerModifySerializeLocalState(
                localState, reason);
        if (payload.empty()) return false;
        state.stateExtension.payload = payload;
        stateChanged = true;
    }
    else if (recovery.decision !=
        BackendAgentNativeTimerModifyRecoveryDecision::returnPersistedEvidence)
    {
        reason = "native_modify_recovery_decision_invalid";
        return false;
    }

    NativeTimerModifyGenericProjection projection;
    if (!nativeTimerModifyGenericProjection(
            recovery.evidence.outcome, projection))
    {
        reason = "native_modify_outcome_projection_invalid";
        return false;
    }

    if (state.resultPresent)
    {
        if (!genericResultMatchesTimerDeleteEvidence(
                state, projection, recovery.evidence))
        {
            reason = "native_modify_result_evidence_conflict";
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

    reason = "native_modify_local_state_reconciled";
    return true;
}

bool backendAgentNativeTimerModifyCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    if (state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "not_started" || currentTime <= 0 ||
        currentTime > state.assignment.deadline)
    {
        reason = "native_modify_fresh_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerModifyLocalState localState;
    if (!backendAgentNativeTimerModifyPrepareLocalStarting(
            state.assignment, currentTime, localState, reason))
        return false;

    BackendAgentCommandStateExtension extension;
    extension.extensionType =
        kBackendAgentNativeTimerModifyLocalStateExtensionType;
    extension.commandId = state.assignment.commandId;
    extension.requestFingerprint = state.assignment.requestFingerprint;
    extension.payload = backendAgentNativeTimerModifySerializeLocalState(
        localState, reason);
    if (extension.payload.empty() ||
        !backendAgentCommandStateExtensionValidateSupported(
            extension, state.assignment, reason))
    {
        reason = "native_modify_fresh_starting_extension_invalid";
        return false;
    }

    state.stateExtensionPresent = true;
    state.stateExtension = extension;
    state.dispatchState = "starting";
    if (!persist(statePath, state, reason)) return false;
    reason = "native_modify_local_starting_persisted";
    return true;
}

bool backendAgentNativeTimerModifyCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerModifyCommandContext& context,
    IBackendAgentNativeTimerModifyTransport* transport,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (transport == nullptr ||
        !state.stateExtensionPresent || state.resultPresent ||
        state.dispatchState != "starting" || !state.receiptAcknowledged ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerModifyLocalStateExtensionType)
    {
        reason = "native_modify_executor_handoff_state_invalid";
        return false;
    }

    BackendAgentNativeTimerModifyLocalState localState;
    if (!backendAgentNativeTimerModifyParseLocalState(
            state.stateExtension.payload, localState, reason) ||
        localState.phase != BackendAgentNativeTimerModifyLocalPhase::starting)
    {
        reason = "native_modify_executor_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerModifyExecutorContext executorContext;
    executorContext.backendId = context.backendId;
    executorContext.agentId = context.agentId;
    executorContext.agentInstanceId = context.agentInstanceId;
    executorContext.backendGeneration = context.backendGeneration;
    executorContext.now = nowSeconds();

    BackendAgentNativeTimerModifyEvidence evidence;
    if (!backendAgentNativeTimerModifyExecuteFreshStartingOnce(
            state.assignment,
            localState,
            executorContext,
            *transport,
            evidence,
            reason))
        return false;

    if (!backendAgentNativeTimerModifyCompleteLocalState(
            localState, evidence, reason))
        return false;
    const std::string payload = backendAgentNativeTimerModifySerializeLocalState(
        localState, reason);
    if (payload.empty()) return false;
    state.stateExtension.payload = payload;

    NativeTimerModifyGenericProjection projection;
    if (!nativeTimerModifyGenericProjection(evidence.outcome, projection))
    {
        reason = "native_modify_executor_outcome_projection_invalid";
        return false;
    }
    createTimerDeleteResult(state, projection, evidence.completedAt);

    if (!persist(statePath, state, reason)) return false;
    reason = "native_modify_executor_outcome_persisted";
    return true;
}

}
