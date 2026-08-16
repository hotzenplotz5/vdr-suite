#include "BackendAgentCommandClient.h"

#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeProbeCommandHandler.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateCommandHandler.h"
#include "BackendAgentNativeTimerDeleteCommandHandler.h"
#include "BackendAgentNativeTimerModifyCommandHandler.h"

#include <chrono>
#include <string>
#include <vector>

namespace
{
using vdrsuite::agent::commandstate::LocalState;
using vdrsuite::agent::commandstate::load;
using vdrsuite::agent::commandstate::persist;
using vdrsuite::agent::commandstate::retireProtectedState;

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

vdrsuite::agent::BackendAgentNativeTimerCreateCommandContext
nativeTimerCreateContext(const BackendAgentCommandClientContext& context)
{
    return {
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration};
}

bool reconcileNativeTimerCreateLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerCreateCommandReconcileExisting(
        config.statePath,
        nativeTimerCreateContext(context),
        state,
        reason);
}

bool prepareFreshNativeTimerCreateLocalStarting(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerCreateCommandPrepareFreshStarting(
        config.statePath,
        state,
        currentTime,
        reason);
}

bool executeFreshNativeTimerCreateAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome(
            config.statePath,
            nativeTimerCreateContext(context),
            config.nativeTimerCreateTransport,
            state,
            reason);
}

vdrsuite::agent::BackendAgentNativeTimerDeleteCommandContext
nativeTimerDeleteContext(const BackendAgentCommandClientContext& context)
{
    return {
        context.backendId,
        context.agentId,
        context.agentInstanceId,
        context.backendGeneration};
}

bool reconcileNativeTimerDeleteLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerDeleteCommandReconcileExisting(
        config.statePath,
        nativeTimerDeleteContext(context),
        state,
        reason);
}

bool prepareFreshNativeTimerDeleteLocalStarting(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerDeleteCommandPrepareFreshStarting(
        config.statePath,
        state,
        currentTime,
        reason);
}

bool executeFreshNativeTimerDeleteAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentNativeTimerDeleteCommandExecuteFreshStartingAndPersistOutcome(
            config.statePath,
            nativeTimerDeleteContext(context),
            config.nativeTimerDeleteTransport,
            state,
            reason);
}


vdrsuite::agent::BackendAgentNativeTimerModifyCommandContext
nativeTimerModifyContext(const BackendAgentCommandClientContext& context)
{
    return {context.backendId, context.agentId, context.agentInstanceId,
            context.backendGeneration};
}
bool reconcileNativeTimerModifyLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state, std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerModifyCommandReconcileExisting(
        config.statePath, nativeTimerModifyContext(context), state, reason);
}
bool prepareFreshNativeTimerModifyLocalStarting(
    const BackendAgentCommandClientConfig& config, LocalState& state,
    std::int64_t currentTime, std::string& reason)
{
    return vdrsuite::agent::backendAgentNativeTimerModifyCommandPrepareFreshStarting(
        config.statePath, state, currentTime, reason);
}
bool executeFreshNativeTimerModifyAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state, std::string& reason)
{
    return vdrsuite::agent::
        backendAgentNativeTimerModifyCommandExecuteFreshStartingAndPersistOutcome(
            config.statePath, nativeTimerModifyContext(context),
            config.nativeTimerModifyTransport, state, reason);
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
        if (type == vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType)
            continue;
        if (type != "vdr.native.probe")
        {
            availability.commandTypes.push_back(type);
            continue;
        }
        vdrsuite::agent::BackendAgentLocalProviderFacts facts;
        std::string reason;
        if (!vdrsuite::agent::backendAgentNativeProbeCommandAvailability(
                config.nativeProbeTransport, facts, reason))
            continue;
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
    const bool timerCreateCommand = state.assignment.commandType ==
        vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType;
    const bool timerDeleteCommand = state.assignment.commandType ==
        vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType;
    const bool timerModifyCommand =
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType;
    if (timerCreateCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerCreateLocalState(config, context, state, reason))
        return false;
    if (timerDeleteCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerDeleteLocalState(config, context, state, reason))
        return false;
    if (timerModifyCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerModifyLocalState(config, context, state, reason))
        return false;
    if (!sameContext(state.assignment, context))
    {
        if (state.resultPresent && state.receiptAcknowledged &&
            state.resultAcknowledged)
            return retireProtectedState(config.statePath, reason);
        reason = "local_command_generation_fenced";
        return false;
    }

    if (timerCreateCommand && !state.stateExtensionPresent && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            reason = "native_create_fresh_starting_state_invalid";
            return false;
        }
        const std::int64_t currentTime = nowSeconds();
        if (state.assignment.deadline <= currentTime)
        {
        
    if (timerModifyCommand && !state.stateExtensionPresent && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            reason = "native_modify_fresh_starting_state_invalid";
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
        if (!prepareFreshNativeTimerModifyLocalStarting(
                config, state, currentTime, reason))
            return false;
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (config.nativeTimerModifyTransport == nullptr)
        {
            reason = "native_modify_local_starting_handoff_persisted";
            return true;
        }
        if (!executeFreshNativeTimerModifyAndPersistOutcome(
                config, context, state, reason))
            return false;
        if (!sendResult(config, context, transport, state, reason))
            return false;
        reason = "native_modify_executor_outcome_reconciled";
        return true;
    }

    if (!state.receiptAcknowledged &&
                !sendReceipt(config, context, transport, state, reason))
                return false;
            createResult(
                state, "not_started", "outcome_unknown", "rejected",
                "expired", "none",
                "command deadline expired before native dispatch");
            if (!persist(config.statePath, state, reason)) return false;
            if (!sendResult(config, context, transport, state, reason)) return false;
            reason = "command_result_reconciled";
            return true;
        }
        if (!prepareFreshNativeTimerCreateLocalStarting(
                config, state, currentTime, reason))
            return false;
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (config.nativeTimerCreateTransport == nullptr)
        {
            reason = "native_create_local_starting_handoff_persisted";
            return true;
        }
        if (!executeFreshNativeTimerCreateAndPersistOutcome(
                config, context, state, reason))
            return false;
        if (!sendResult(config, context, transport, state, reason))
            return false;
        reason = "native_create_executor_outcome_reconciled";
        return true;
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
        if (!vdrsuite::agent::backendAgentNativeProbeCommandReconcile(
                config.statePath,
                config.nativeProbeTransport,
                state,
                reason))
            return false;
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
    vdrsuite::agent::backendAgentNativeProbeCommandSetDefaultTransport(transport);
}
