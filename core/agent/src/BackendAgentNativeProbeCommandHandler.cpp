#include "BackendAgentNativeProbeCommandHandler.h"

#include <chrono>

namespace
{
using vdrsuite::agent::commandstate::LocalState;
using vdrsuite::agent::commandstate::persist;

vdrsuite::agent::IBackendAgentNativeProbeTransport* GlobalNativeProbeTransport = nullptr;

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

vdrsuite::agent::IBackendAgentNativeProbeTransport* nativeProbeTransport(
    vdrsuite::agent::IBackendAgentNativeProbeTransport* configuredTransport)
{
    return configuredTransport != nullptr
        ? configuredTransport : GlobalNativeProbeTransport;
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

bool negotiateNativeCapability(
    vdrsuite::agent::IBackendAgentNativeProbeTransport* configuredTransport,
    vdrsuite::agent::SuiteBridgeNativeProbeCapability& capability,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    IBackendAgentNativeProbeTransport* transport =
        nativeProbeTransport(configuredTransport);
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
    const std::string& statePath,
    vdrsuite::agent::IBackendAgentNativeProbeTransport* configuredTransport,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    SuiteBridgeNativeProbeCapability capability;
    if (!negotiateNativeCapability(configuredTransport, capability, reason) ||
        !nativeCapabilityMatchesAssignment(state, capability, reason))
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "fenced", "reconcile_only",
            "native probe provider fence changed before readback");
        return persist(statePath, state, reason);
    }
    if (state.nativeExecutionSequence == 0)
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe sequence unavailable for readback");
        return persist(statePath, state, reason);
    }
    SuiteBridgeNativeProbeReadbackRequest readbackRequest;
    readbackRequest.commandId = state.assignment.commandId;
    readbackRequest.requestFingerprint = state.assignment.requestFingerprint;
    readbackRequest.pluginInstanceEpoch = state.pluginInstanceEpoch;
    readbackRequest.nativeExecutionSequence = state.nativeExecutionSequence;
    IBackendAgentNativeProbeTransport* transport =
        nativeProbeTransport(configuredTransport);
    const SuiteBridgeCommandReply reply =
        transport->readNativeProbe(readbackRequest);
    if (!reply.transportSucceeded())
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "executor_unknown", "reconcile_only",
            "native probe readback transport outcome unknown");
        return persist(statePath, state, reason);
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
        return persist(statePath, state, reason);
    }
    state.nativeReadbackEvidence =
        backendAgentNativeProbeReadbackEvidence(evidence);
    createResult(
        state, "effect_reported", "verified", "succeeded", "none", "none",
        "vdr.native.probe verified with mutations disabled");
    return persist(statePath, state, reason);
}

bool executeOrRecoverNative(
    const std::string& statePath,
    vdrsuite::agent::IBackendAgentNativeProbeTransport* configuredTransport,
    LocalState& state,
    std::string& reason)
{
    using namespace vdrsuite::agent;
    SuiteBridgeNativeProbeCapability capability;
    if (!negotiateNativeCapability(configuredTransport, capability, reason))
    {
        createResult(
            state, "not_started", "outcome_unknown", "rejected",
            "unsupported", "none", "native probe capability unavailable");
        return persist(statePath, state, reason);
    }

    const bool recoveringDispatch = !state.pluginInstanceEpoch.empty();
    if (state.pluginInstanceEpoch.empty())
    {
        if (state.assignment.payloadVersion != 2)
        {
            createResult(
                state, "not_started", "outcome_unknown", "rejected",
                "fenced", "none", "native probe provider selection required");
            return persist(statePath, state, reason);
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
            return persist(statePath, state, reason);
        }
        state.probeNonce = payload.probeNonce;
        state.pluginInstanceEpoch =
            payload.localProviderSelection.providerInstanceEpoch;
        state.nativeCapabilityEvidence =
            backendAgentNativeProbeCapabilityEvidence(capability);
        state.dispatchState="starting";
        if (!persist(statePath, state, reason)) return false;
    }
    else if (!nativeCapabilityMatchesAssignment(state, capability, reason))
    {
        createResult(
            state, state.dispatchState, "outcome_unknown", "outcome_unknown",
            "fenced", "reconcile_only",
            "native probe selected provider cannot be replayed");
        return persist(statePath, state, reason);
    }

    const SuiteBridgeNativeProbeRequest request = nativeRequest(state);
    IBackendAgentNativeProbeTransport* transport =
        nativeProbeTransport(configuredTransport);
    const SuiteBridgeCommandReply reply = transport->executeNativeProbe(request);
    if (!reply.transportSucceeded())
    {
        if (!recoveringDispatch)
        {
            std::string persistReason;
            if (!persist(statePath, state, persistReason))
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
        return persist(statePath, state, reason);
    }
    if (reply.replyCode != 900)
    {
        createResult(
            state, "starting", "outcome_unknown", "rejected",
            reply.replyCode == 555 || reply.replyCode == 554
                ? "fenced" : "unsupported",
            "none", "native probe executor rejected request");
        return persist(statePath, state, reason);
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
        return persist(statePath, state, reason);
    }

    state.nativeExecutionSequence = evidence.nativeExecutionSequence;
    state.nativeReceiptEvidence =
        backendAgentNativeProbeReceiptEvidence(evidence);
    state.dispatchState = "accepted_by_executor";
    if (!persist(statePath, state, reason)) return false;
    state.nativeResultEvidence =
        backendAgentNativeProbeResultEvidence(evidence);
    state.dispatchState = "effect_reported";
    if (!persist(statePath, state, reason)) return false;
    return completeNativeReadback(
        statePath, configuredTransport, state, reason);
}
}

namespace vdrsuite::agent
{

void backendAgentNativeProbeCommandSetDefaultTransport(
    IBackendAgentNativeProbeTransport* transport)
{
    GlobalNativeProbeTransport = transport;
}

bool backendAgentNativeProbeCommandAvailability(
    IBackendAgentNativeProbeTransport* configuredTransport,
    BackendAgentLocalProviderFacts& facts,
    std::string& reason)
{
    SuiteBridgeNativeProbeCapability capability;
    if (!negotiateNativeCapability(
            configuredTransport, capability, reason))
        return false;
    facts = backendAgentNativeProbeProviderFacts(capability);
    return backendAgentLocalProviderValidFacts(facts);
}

bool backendAgentNativeProbeCommandReconcile(
    const std::string& statePath,
    IBackendAgentNativeProbeTransport* configuredTransport,
    commandstate::LocalState& state,
    std::string& reason)
{
    if (state.dispatchState == "not_started" ||
        state.dispatchState == "starting")
    {
        return executeOrRecoverNative(
            statePath, configuredTransport, state, reason);
    }
    if (state.dispatchState == "accepted_by_executor")
    {
        return executeOrRecoverNative(
            statePath, configuredTransport, state, reason);
    }
    if (state.dispatchState == "effect_reported")
    {
        return completeNativeReadback(
            statePath, configuredTransport, state, reason);
    }
    reason = "unsupported_local_command_state";
    return false;
}

}
