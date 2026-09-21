#include "BackendAgentNativeTimerCreateCommandHandler.h"

#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerCreateLocalState.h"

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

struct NativeTimerCreateGenericProjection
{
    std::string dispatchState;
    std::string verificationState;
    std::string resultCategory;
    std::string errorCategory;
    std::string retryClassification;
    std::string diagnostics;
};

bool nativeTimerCreateGenericProjection(
    vdrsuite::agent::BackendAgentNativeTimerCreateOutcomeCategory outcome,
    NativeTimerCreateGenericProjection& projection)
{
    using namespace vdrsuite::agent;

    switch (outcome)
    {
        case BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect:
            projection = {
                "not_started",
                "verified",
                "rejected",
                "fenced",
                "none",
                "native Timer create rejected without effect"};
            return true;

        case BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified:
            projection = {
                "accepted_by_executor",
                "outcome_unknown",
                "outcome_unknown",
                "none",
                "reconcile_only",
                "native Timer create accepted; readback reconciliation required"};
            return true;

        case BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown:
            projection = {
                "starting",
                "outcome_unknown",
                "outcome_unknown",
                "executor_unknown",
                "reconcile_only",
                "native Timer create outcome unknown; reconciliation required"};
            return true;
    }

    return false;
}

void createTimerCreateResult(
    LocalState& state,
    const NativeTimerCreateGenericProjection& projection,
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

bool genericResultMatchesTimerCreateEvidence(
    const LocalState& state,
    const NativeTimerCreateGenericProjection& projection,
    const vdrsuite::agent::BackendAgentNativeTimerCreateEvidence& evidence)
{
    if (!state.resultPresent)
        return false;

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

} // namespace

namespace vdrsuite::agent
{

bool backendAgentNativeTimerCreateCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerCreateCommandContext& context,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (!state.stateExtensionPresent ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerCreateLocalStateExtensionType)
    {
        reason = "native_create_local_state_required";
        return false;
    }

    BackendAgentNativeTimerCreateLocalState localState;

    if (!backendAgentNativeTimerCreateParseLocalState(
            state.stateExtension.payload, localState, reason))
    {
        reason = "native_create_local_state_invalid";
        return false;
    }

    const auto recovery = backendAgentNativeTimerCreateRecoverLocalState(
        localState,
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration,
        nowSeconds());

    if (recovery.decision ==
        BackendAgentNativeTimerCreateRecoveryDecision::failClosed)
    {
        reason = recovery.reasonCode.empty()
            ? "native_create_recovery_failed"
            : recovery.reasonCode;
        return false;
    }

    bool stateChanged = false;

    if (recovery.decision ==
        BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly)
    {
        if (!backendAgentNativeTimerCreateCompleteLocalState(
                localState, recovery.evidence, reason))
            return false;

        const std::string payload =
            backendAgentNativeTimerCreateSerializeLocalState(
                localState, reason);

        if (payload.empty())
            return false;

        state.stateExtension.payload = payload;
        stateChanged = true;
    }
    else if (recovery.decision !=
        BackendAgentNativeTimerCreateRecoveryDecision::returnPersistedEvidence)
    {
        reason = "native_create_recovery_decision_invalid";
        return false;
    }

    NativeTimerCreateGenericProjection projection;

    if (!nativeTimerCreateGenericProjection(
            recovery.evidence.outcome, projection))
    {
        reason = "native_create_outcome_projection_invalid";
        return false;
    }

    if (state.resultPresent)
    {
        if (!genericResultMatchesTimerCreateEvidence(
                state, projection, recovery.evidence))
        {
            reason = "native_create_result_evidence_conflict";
            return false;
        }
    }
    else
    {
        createTimerCreateResult(
            state, projection, recovery.evidence.completedAt);
        stateChanged = true;
    }

    if (stateChanged)
        return persist(statePath, state, reason);

    reason = "native_create_local_state_reconciled";
    return true;
}

bool backendAgentNativeTimerCreateCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    if (state.stateExtensionPresent ||
        state.resultPresent ||
        state.dispatchState != "not_started" ||
        currentTime <= 0 ||
        currentTime > state.assignment.deadline)
    {
        reason = "native_create_fresh_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerCreateCommand command;

    if (!backendAgentNativeTimerCreateCommandFromAssignment(
            state.assignment, command, reason))
    {
        reason = "native_create_fresh_assignment_invalid";
        return false;
    }

    BackendAgentNativeTimerCreateLocalState localState;

    if (!backendAgentNativeTimerCreatePrepareLocalStarting(
            command, currentTime, localState, reason))
        return false;

    BackendAgentCommandStateExtension extension;
    extension.extensionType =
        kBackendAgentNativeTimerCreateLocalStateExtensionType;
    extension.commandId = state.assignment.commandId;
    extension.requestFingerprint = state.assignment.requestFingerprint;
    extension.payload =
        backendAgentNativeTimerCreateSerializeLocalState(
            localState, reason);

    if (extension.payload.empty() ||
        !backendAgentCommandStateExtensionValidateSupported(
            extension, state.assignment, reason))
    {
        reason = "native_create_fresh_starting_extension_invalid";
        return false;
    }

    state.stateExtensionPresent = true;
    state.stateExtension = extension;
    state.dispatchState = "starting";

    if (!persist(statePath, state, reason))
        return false;

    reason = "native_create_local_starting_persisted";
    return true;
}

bool backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerCreateCommandContext& context,
    IBackendAgentNativeTimerCreateTransport* transport,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (transport == nullptr ||
        !state.stateExtensionPresent ||
        state.resultPresent ||
        state.dispatchState != "starting" ||
        !state.receiptAcknowledged ||
        state.stateExtension.extensionType !=
            kBackendAgentNativeTimerCreateLocalStateExtensionType)
    {
        reason = "native_create_executor_handoff_state_invalid";
        return false;
    }

    BackendAgentNativeTimerCreateLocalState localState;

    if (!backendAgentNativeTimerCreateParseLocalState(
            state.stateExtension.payload, localState, reason) ||
        localState.phase != BackendAgentNativeTimerCreateLocalPhase::starting)
    {
        reason = "native_create_executor_starting_state_invalid";
        return false;
    }

    BackendAgentNativeTimerCreateExecutorContext executorContext;
    executorContext.backendId = context.backendId;
    executorContext.agentId = context.agentId;
    executorContext.agentInstanceId = context.agentInstanceId;
    executorContext.backendGeneration = context.backendGeneration;
    executorContext.now = nowSeconds();

    BackendAgentNativeTimerCreateEvidence evidence;

    if (!backendAgentNativeTimerCreateExecuteFreshStartingOnce(
            state.assignment,
            localState,
            executorContext,
            *transport,
            evidence,
            reason))
        return false;

    if (!backendAgentNativeTimerCreateCompleteLocalState(
            localState, evidence, reason))
        return false;

    const std::string payload =
        backendAgentNativeTimerCreateSerializeLocalState(
            localState, reason);

    if (payload.empty())
        return false;

    state.stateExtension.payload = payload;

    NativeTimerCreateGenericProjection projection;

    if (!nativeTimerCreateGenericProjection(
            evidence.outcome, projection))
    {
        reason = "native_create_executor_outcome_projection_invalid";
        return false;
    }

    createTimerCreateResult(
        state, projection, evidence.completedAt);

    if (!persist(statePath, state, reason))
        return false;

    reason = "native_create_executor_outcome_persisted";
    return true;
}

} // namespace vdrsuite::agent
