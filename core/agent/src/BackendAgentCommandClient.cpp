#include "BackendAgentCommandClient.h"

#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeProbe.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"

#include <chrono>
#include <string>
#include <vector>

namespace
{
using vdrsuite::agent::commandstate::LocalState;
using vdrsuite::agent::commandstate::load;
using vdrsuite::agent::commandstate::persist;
using vdrsuite::agent::commandstate::retireProtectedState;

vdrsuite::agent::IBackendAgentNativeProbeTransport* GlobalNativeProbeTransport = nullptr;

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool sameContext(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandClientContext& context)
{
    return assignment.backendId == context.backendId &&
        assignment.agentId == context.agentId &&
        assignment.agentInstanceId == context.agentInstanceId &&
        assignment.backendGeneration == context.backendGeneration;
}

std::string responseCode(const BackendAgentTransportResponse& response)
{
    return response.errorCode.empty()
        ? "command_transport_failed" : response.errorCode;
}

bool sendReceipt(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    LocalState& state,
    std::string& reason)
{
    const auto response = transport.postAuthenticated(
        context.agentId, context.credentialSecret,
        "/api/agent/v1/commands/receipt",
        serializeBackendAgentCommandReceiptJson(state.receipt));
    if (!response.transportSucceeded || response.statusCode != 200)
    {
        reason = responseCode(response);
        return false;
    }
    state.receiptAcknowledged = true;
    return persist(config.statePath, state, reason);
}

bool sendResult(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    LocalState& state,
    std::string& reason)
{
    const auto response = transport.postAuthenticated(
        context.agentId, context.credentialSecret,
        "/api/agent/v1/commands/result",
        serializeBackendAgentCommandResultJson(state.result));
    if (!response.transportSucceeded || response.statusCode != 200)
    {
        reason = responseCode(response);
        return false;
    }
    state.resultAcknowledged = true;
    return persist(config.statePath, state, reason);
}

void createResult(
    LocalState& state,
    const std::string& dispatch,
    const std::string& verification,
    const std::string& category,
    const std::string& error,
    const std::string& retry,
    const std::string& diagnostics)
{
    state.dispatchState = dispatch;
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
    result.dispatchState = dispatch;
    result.verificationState = verification;
    result.resultCategory = category;
    result.errorCategory = error;
    result.retryClassification = retry;
    result.boundedDiagnostics = diagnostics;
    result.completedAt = nowSeconds();
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

bool reconcileNativeTimerDeleteLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
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
        createResult(
            state,
            projection.dispatchState,
            projection.verificationState,
            projection.resultCategory,
            projection.errorCategory,
            projection.retryClassification,
            projection.diagnostics);
        state.result.completedAt = recovery.evidence.completedAt;
        stateChanged = true;
    }

    if (stateChanged)
        return persist(config.statePath, state, reason);

    reason = "native_delete_local_state_reconciled";
    return true;
}

bool prepareFreshNativeTimerDeleteLocalStarting(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    using namespace vdrsuite::agent;
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
    if (!persist(config.statePath, state, reason)) return false;
    reason = "native_delete_local_starting_persisted";
    return true;
}

bool executeFreshNativeTimerDeleteAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    if (config.nativeTimerDeleteTransport == nullptr ||
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
            *config.nativeTimerDeleteTransport,
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
    createResult(
        state,
        projection.dispatchState,
        projection.verificationState,
        projection.resultCategory,
        projection.errorCategory,
        projection.retryClassification,
        projection.diagnostics);
    state.result.completedAt = evidence.completedAt;

    if (!persist(config.statePath, state, reason)) return false;
    reason = "native_delete_executor_outcome_persisted";
    return true;
}

bool negotiateNativeCapability(
    const BackendAgentCommandClientConfig& config,
    vdrsuite::agent::SuiteBridgeNativeProbeCapability& capability,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    IBackendAgentNativeProbeTransport* transport = config.nativeProbeTransport != nullptr
        ? config.nativeProbeTransport : GlobalNativeProbeTransport;
    if (transport == nullptr)
    {
        reason = "native_capability_unavailable";
        return false;
    }
    const SuiteBridgeCommandReply reply = transport->discoverNativeProbe();
    if (!reply.transportSucceeded() || reply.replyCode != 900 ||
        !backendAgentNativeProbeParseCapability(
            reply.payload, capability, reason))
    {
        reason = "native_capability_unavailable";
        return false;
    }
    reason = "native_capability_available";
    return true;
}

bool nativeCapabilityMatchesAssignment(
    const LocalState& state,
    const vdrsuite::agent::SuiteBridgeNativeProbeCapability& capability,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    if (state.assignment.payloadVersion == 2)
    {
        BackendAgentNativeProbePayload payload;
        if (!backendAgentNativeProbeParseSelectedPayload(
                state.assignment.payload, payload, reason)) return false;
        if (!backendAgentNativeProbeSelectionMatchesCapability(
                payload.localProviderSelection,
                state.assignment.backendId,
                capability,
                reason)) return false;
        if (!state.pluginInstanceEpoch.empty() &&
            state.pluginInstanceEpoch !=
                payload.localProviderSelection.providerInstanceEpoch)
        {
            reason = "local_provider_instance_epoch_changed";
            return false;
        }
        if (!state.probeNonce.empty() && state.probeNonce != payload.probeNonce)
        {
            reason = "native_probe_payload_changed";
            return false;
        }
        return true;
    }
    if (state.assignment.payloadVersion == 1 &&
        !state.pluginInstanceEpoch.empty() &&
        state.pluginInstanceEpoch == capability.pluginInstanceEpoch)
    {
        reason = "legacy_native_probe_reconciliation_only";
        return true;
    }
    reason = "native_probe_provider_selection_required";
    return false;
}

vdrsuite::agent::SuiteBridgeNativeProbeRequest nativeRequest(
    const LocalState& state)
{
    vdrsuite::agent::SuiteBridgeNativeProbeRequest request;
    const auto& assignment = state.assignment;
    request.commandId = assignment.commandId;
    request.requestFingerprint = assignment.requestFingerprint;
    request.operationId = assignment.operationId;
    request.jobId = assignment.jobId;
    request.attemptId = assignment.attemptId;
    request.claimEpoch = assignment.claimEpoch;
    request.backendId = assignment.backendId;
    request.agentId = assignment.agentId;
    request.agentInstanceId = assignment.agentInstanceId;
    request.backendGeneration = assignment.backendGeneration;
    request.pluginInstanceEpoch = state.pluginInstanceEpoch;
    request.probeNonce = state.probeNonce;
    return request;
}

bool completeNativeReadback(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    SuiteBridgeNativeProbeCapability capability;
    if (!negotiateNativeCapability(config, capability, reason) ||
        !nativeCapabilityMatchesAssignment(state, capability, reason))
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "fenced", "reconcile_only",
            "native probe provider fence changed before readback");
        return persist(config.statePath, state, reason);
    }
    if (state.nativeExecutionSequence == 0)
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe sequence unavailable for readback");
        return persist(config.statePath, state, reason);
    }
    SuiteBridgeNativeProbeReadbackRequest readbackRequest;
    readbackRequest.commandId = state.assignment.commandId;
    readbackRequest.requestFingerprint = state.assignment.requestFingerprint;
    readbackRequest.pluginInstanceEpoch = state.pluginInstanceEpoch;
    readbackRequest.nativeExecutionSequence = state.nativeExecutionSequence;
    const SuiteBridgeCommandReply reply =
        (config.nativeProbeTransport != nullptr ? config.nativeProbeTransport : GlobalNativeProbeTransport)->readNativeProbe(readbackRequest);
    if (!reply.transportSucceeded())
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe readback transport outcome unknown");
        return persist(config.statePath, state, reason);
    }
    SuiteBridgeNativeProbeEvidence evidence;
    const auto request = nativeRequest(state);
    if (reply.replyCode != 900 ||
        !backendAgentNativeProbeParseEvidence(
            reply.payload, true, evidence, reason) ||
        !backendAgentNativeProbeEvidenceMatches(evidence, request, true) ||
        evidence.nativeExecutionSequence != state.nativeExecutionSequence)
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "rejected",
            reply.replyCode == 555 ? "fenced" : "executor_unknown",
            "reconcile_only", "native probe readback verification failed");
        return persist(config.statePath, state, reason);
    }
    state.nativeReadbackEvidence =
        backendAgentNativeProbeReadbackEvidence(evidence);
    createResult(
        state, "effect_reported", "verified", "succeeded", "none", "none",
        "vdr.native.probe verified with mutations disabled");
    return persist(config.statePath, state, reason);
}

bool executeOrRecoverNative(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    SuiteBridgeNativeProbeCapability capability;
    if (!negotiateNativeCapability(config, capability, reason))
    {
        createResult(
            state, "not_started", "outcome_unknown", "rejected",
            "unsupported", "none", "native probe capability unavailable");
        return persist(config.statePath, state, reason);
    }

    const bool recoveringDispatch = !state.pluginInstanceEpoch.empty();
    if (state.pluginInstanceEpoch.empty())
    {
        if (state.assignment.payloadVersion != 2)
        {
            createResult(
                state, "not_started", "outcome_unknown", "rejected",
                "fenced", "none", "native probe provider selection required");
            return persist(config.statePath, state, reason);
        }
        BackendAgentNativeProbePayload payload;
        if (!backendAgentNativeProbeParseSelectedPayload(
                state.assignment.payload, payload, reason) ||
            !backendAgentNativeProbeSelectionMatchesCapability(
                payload.localProviderSelection,
                state.assignment.backendId,
                capability,
                reason))
        {
            createResult(
                state, "not_started", "outcome_unknown", "rejected",
                "fenced", "none", "native probe selected provider unavailable");
            return persist(config.statePath, state, reason);
        }
        state.probeNonce = payload.probeNonce;
        state.pluginInstanceEpoch =
            payload.localProviderSelection.providerInstanceEpoch;
        state.nativeCapabilityEvidence =
            backendAgentNativeProbeCapabilityEvidence(capability);
        state.dispatchState="starting";
        if (!persist(config.statePath, state, reason)) return false;
    }
    else if (!nativeCapabilityMatchesAssignment(state, capability, reason))
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "fenced", "reconcile_only",
            "native probe selected provider cannot be replayed");
        return persist(config.statePath, state, reason);
    }

    const SuiteBridgeNativeProbeRequest request = nativeRequest(state);
    const SuiteBridgeCommandReply reply =
        (config.nativeProbeTransport != nullptr ? config.nativeProbeTransport : GlobalNativeProbeTransport)->executeNativeProbe(request);
    if (!reply.transportSucceeded())
    {
        if (!recoveringDispatch)
        {
            std::string persistReason;
            if (!persist(config.statePath, state, persistReason))
            {
                reason = persistReason;
                return false;
            }
            reason = "native_probe_dispatch_reconciliation_required";
            return false;
        }
        createResult(
            state, "starting", "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe dispatch remained unknown after exact replay");
        return persist(config.statePath, state, reason);
    }
    if (reply.replyCode != 900)
    {
        createResult(
            state, "starting", "outcome_unknown", "rejected",
            reply.replyCode == 555 || reply.replyCode == 554
                ? "fenced" : "unsupported",
            "none", "native probe executor rejected request");
        return persist(config.statePath, state, reason);
    }

    SuiteBridgeNativeProbeEvidence evidence;
    if (!backendAgentNativeProbeParseEvidence(
            reply.payload, false, evidence, reason) ||
        !backendAgentNativeProbeEvidenceMatches(evidence, request, false))
    {
        createResult(
            state, "starting", "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe executor evidence invalid");
        return persist(config.statePath, state, reason);
    }

    state.nativeExecutionSequence = evidence.nativeExecutionSequence;
    state.nativeReceiptEvidence =
        backendAgentNativeProbeReceiptEvidence(evidence);
    state.dispatchState = "accepted_by_executor";
    if (!persist(config.statePath, state, reason)) return false;
    state.nativeResultEvidence =
        backendAgentNativeProbeResultEvidence(evidence);
    state.dispatchState = "effect_reported";
    if (!persist(config.statePath, state, reason)) return false;
    return completeNativeReadback(config, state, reason);
}

bool reconcileNative(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::string& reason)
{
    if (state.dispatchState == "not_started" ||
        state.dispatchState == "starting")
    {
        return executeOrRecoverNative(config, state, reason);
    }
    if (state.dispatchState == "accepted_by_executor")
    {
        return executeOrRecoverNative(config, state, reason);
    }
    if (state.dispatchState == "effect_reported")
    {
        return completeNativeReadback(config, state, reason);
    }
    reason = "unsupported_local_command_state";
    return false;
}

struct CommandAvailability
{
    std::vector<std::string> commandTypes;
    std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts> localProviders;
};

CommandAvailability availableCommands(
    const BackendAgentCommandClientConfig& config)
{
    CommandAvailability availability;
    for (const std::string& type : config.commandTypes)
    {
        if (type ==
            vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType)
            continue;
        if (type != "vdr.native.probe")
        {
            availability.commandTypes.push_back(type);
            continue;
        }
        vdrsuite::agent::SuiteBridgeNativeProbeCapability capability;
        std::string reason;
        if (!negotiateNativeCapability(config, capability, reason)) continue;
        const auto facts =
            vdrsuite::agent::backendAgentNativeProbeProviderFacts(capability);
        if (!vdrsuite::agent::backendAgentLocalProviderValidFacts(facts)) continue;
        availability.commandTypes.push_back(type);
        availability.localProviders.push_back(facts);
    }
    return availability;
}
}

bool reconcileBackendAgentCommandState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    std::string& reason)
{
    if (config.commandTypes.empty())
    {
        reason = "command_delivery_disabled";
        return true;
    }
    LocalState state;
    if (!load(config.statePath, state, reason))
    {
        if (reason == "command_state_not_found")
        {
            reason = "no_local_command";
            return true;
        }
        return false;
    }
    const bool timerDeleteCommand = state.assignment.commandType ==
        vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType;
    if (timerDeleteCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerDeleteLocalState(
            config, context, state, reason))
        return false;
    if (!sameContext(state.assignment, context))
    {
        if (state.resultPresent && state.receiptAcknowledged &&
            state.resultAcknowledged)
            return retireProtectedState(config.statePath, reason);
        reason = "local_command_generation_fenced";
        return false;
    }

    if (timerDeleteCommand && !state.stateExtensionPresent && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            reason = "native_delete_fresh_starting_state_invalid";
            return false;
        }
        const std::int64_t currentTime = nowSeconds();
        if (state.assignment.deadline <= currentTime)
        {
            if (!state.receiptAcknowledged &&
                !sendReceipt(config, context, transport, state, reason))
                return false;
            createResult(
                state, "not_started", "outcome_unknown", "rejected",
                "expired", "none", "command deadline expired before native dispatch");
            if (!persist(config.statePath, state, reason)) return false;
            if (!sendResult(config, context, transport, state, reason)) return false;
            reason = "command_result_reconciled";
            return true;
        }
        if (!prepareFreshNativeTimerDeleteLocalStarting(
                config, state, currentTime, reason))
            return false;
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (config.nativeTimerDeleteTransport == nullptr)
        {
            reason = "native_delete_local_starting_handoff_persisted";
            return true;
        }
        if (!executeFreshNativeTimerDeleteAndPersistOutcome(
                config, context, state, reason))
            return false;
        if (!sendResult(config, context, transport, state, reason))
            return false;
        reason = "native_delete_executor_outcome_reconciled";
        return true;
    }

    if (!state.receiptAcknowledged &&
        !sendReceipt(config, context, transport, state, reason)) return false;
    if (state.resultPresent)
    {
        if (!state.resultAcknowledged &&
            !sendResult(config, context, transport, state, reason)) return false;
        reason = "command_result_reconciled";
        return true;
    }

    if (state.assignment.deadline <= nowSeconds() &&
        state.dispatchState == "not_started")
    {
        createResult(
            state, "not_started", "outcome_unknown", "rejected",
            "expired", "none", "command deadline expired before native dispatch");
        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
    }

    if (state.assignment.commandType == "vdr.native.probe")
    {
        if (!reconcileNative(config, state, reason)) return false;
        return state.resultPresent
            ? sendResult(config, context, transport, state, reason) : true;
    }

    if (state.dispatchState == "starting" ||
        state.dispatchState == "accepted_by_executor")
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "probe execution boundary recovered without re-execution");
        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
    }
    if (state.dispatchState != "not_started" ||
        state.assignment.commandType != "probe.noop")
    {
        reason = "unsupported_local_command_state";
        return false;
    }
    state.dispatchState="starting";
    if (!persist(config.statePath, state, reason)) return false;
    state.dispatchState = "accepted_by_executor";
    if (!persist(config.statePath, state, reason)) return false;
    createResult(
        state, "effect_reported", "not_required", "succeeded", "none", "none",
        "probe.noop completed without native side effect");
    if (!persist(config.statePath, state, reason)) return false;
    return sendResult(config, context, transport, state, reason);
}

bool pollBackendAgentCommand(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    std::string& reason)
{
    if (config.commandTypes.empty())
    {
        reason = "command_delivery_disabled";
        return true;
    }
    if (!reconcileBackendAgentCommandState(
            config, context, transport, reason) &&
        reason != "no_local_command") return false;

    BackendAgentCommandPollRequest request;
    request.backendId = context.backendId;
    request.agentInstanceId = context.agentInstanceId;
    request.backendGeneration = context.backendGeneration;
    const CommandAvailability availability = availableCommands(config);
    request.supportedCommandTypes = availability.commandTypes;
    request.localProviders = availability.localProviders;
    const auto response = transport.postAuthenticated(
        context.agentId, context.credentialSecret,
        "/api/agent/v1/commands/poll",
        serializeBackendAgentCommandPollRequestJson(request));
    if (!response.transportSucceeded || response.statusCode != 200)
    {
        reason = responseCode(response);
        return false;
    }
    BackendAgentCommandPollResult result;
    if (!parseBackendAgentCommandPollResponseJson(
            response.body, result, reason)) return false;
    if (!result.assignment.present)
    {
        reason = request.supportedCommandTypes.empty()
            ? "native_capability_unavailable" : "no_command_available";
        return true;
    }
    if (!sameContext(result.assignment, context))
    {
        reason = "command_assignment_context_mismatch";
        return false;
    }

    LocalState current;
    std::string loadReason;
    if (load(config.statePath, current, loadReason))
    {
        if (current.assignment.commandId == result.assignment.commandId)
        {
            if (current.assignment.requestFingerprint !=
                result.assignment.requestFingerprint)
            {
                reason = "conflicting_duplicate_command";
                return false;
            }
            current.receiptAcknowledged=false;
            if (current.resultPresent) current.resultAcknowledged = false;
            if (!persist(config.statePath, current, reason)) return false;
            return reconcileBackendAgentCommandState(
                config, context, transport, reason);
        }
        if (!current.resultAcknowledged)
        {
            reason = "local_command_inbox_busy";
            return false;
        }
    }
    else if (loadReason != "command_state_not_found")
    {
        reason = loadReason;
        return false;
    }

    LocalState state;
    state.assignment = result.assignment;
    auto& receipt = state.receipt;
    receipt.commandId = result.assignment.commandId;
    receipt.requestFingerprint = result.assignment.requestFingerprint;
    receipt.jobId = result.assignment.jobId;
    receipt.attemptId = result.assignment.attemptId;
    receipt.claimEpoch = result.assignment.claimEpoch;
    receipt.backendId = result.assignment.backendId;
    receipt.agentId = result.assignment.agentId;
    receipt.agentInstanceId = result.assignment.agentInstanceId;
    receipt.backendGeneration = result.assignment.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = nowSeconds();
    receipt.reasonCode = "durably_recorded";
    if (!persist(config.statePath, state, reason)) return false;
    return reconcileBackendAgentCommandState(config, context, transport, reason);
}

void setBackendAgentNativeProbeTransport(
    vdrsuite::agent::IBackendAgentNativeProbeTransport* transport)
{
    GlobalNativeProbeTransport = transport;
}
