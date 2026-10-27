#include "BackendAgentCommandClient.h"

#include "BackendAgentClient.h"
#include "BackendAgentCommandJson.h"
#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeProbeCommandHandler.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateCommandHandler.h"
#include "BackendAgentNativeTimerCreateExecutor.h"
#include "BackendAgentNativeTimerDeleteCommandHandler.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "BackendAgentNativeTimerModifyCommandHandler.h"
#include "BackendAgentNativeTimerModifyExecutor.h"
#include "BackendAgentRecordingMarksModify.h"
#include "BackendAgentRecordingMarksModifyCommandHandler.h"
#include "BackendAgentRecordingMarksModifyTransport.h"
#include "BackendAgentRecordingCut.h"
#include "BackendAgentRecordingCutCommandHandler.h"
#include "BackendAgentRecordingCutTransport.h"
#include "ISuiteBridgeLegacyOsdInputTransport.h"
#include "LegacyOsdInputDomain.h"
#include "MediaToolsAgentExecutor.h"
#include "MediaShrinkAgentExecutor.h"
#include "MediaImportRecordingEvidence.h"

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace
{
using vdrsuite::agent::commandstate::LocalState;
using vdrsuite::agent::commandstate::load;
using vdrsuite::agent::commandstate::persist;
using vdrsuite::agent::commandstate::retireProtectedState;

vdrsuite::agent::IBackendAgentRecordingMarksModifyTransport*
    RecordingMarksModifyTransport = nullptr;

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

vdrsuite::agent::BackendAgentRecordingMarksModifyCommandContext
recordingMarksModifyContext(const BackendAgentCommandClientContext& context)
{
    return {context.backendId, context.agentId, context.agentInstanceId,
            context.backendGeneration};
}

bool reconcileRecordingMarksModifyLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentRecordingMarksModifyCommandReconcileExisting(
            config.statePath,
            recordingMarksModifyContext(context),
            state,
            reason);
}

bool prepareFreshRecordingMarksModifyLocalStarting(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentRecordingMarksModifyCommandPrepareFreshStarting(
            config.statePath,
            state,
            currentTime,
            reason);
}

bool executeFreshRecordingMarksModifyAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentRecordingMarksModifyCommandExecuteFreshStartingAndPersistOutcome(
            config.statePath,
            recordingMarksModifyContext(context),
            RecordingMarksModifyTransport,
            state,
            reason);
}

vdrsuite::agent::BackendAgentRecordingCutCommandContext
recordingCutContext(const BackendAgentCommandClientContext& context)
{
    return {context.backendId, context.agentId, context.agentInstanceId,
            context.backendGeneration};
}

bool reconcileRecordingCutLocalState(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentRecordingCutCommandReconcileExisting(
        config.statePath, recordingCutContext(context), state, reason);
}

bool prepareFreshRecordingCutLocalStarting(
    const BackendAgentCommandClientConfig& config,
    LocalState& state,
    std::int64_t currentTime,
    std::string& reason)
{
    return vdrsuite::agent::backendAgentRecordingCutCommandPrepareFreshStarting(
        config.statePath, state, currentTime, reason);
}

bool executeFreshRecordingCutAndPersistOutcome(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    LocalState& state,
    std::string& reason)
{
    return vdrsuite::agent::
        backendAgentRecordingCutCommandExecuteFreshStartingAndPersistOutcome(
            config.statePath,
            recordingCutContext(context),
            config.recordingCutTransport,
            state,
            reason);
}

using CommandAvailability = BackendAgentCommandAvailabilitySnapshot;

bool hasCommandType(
    const BackendAgentCommandClientConfig& config,
    const char* commandType)
{
    return std::find(
        config.commandTypes.begin(), config.commandTypes.end(), commandType) !=
        config.commandTypes.end();
}

bool hasCapability(
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts,
    const char* capability)
{
    return std::find(
        facts.capabilities.begin(), facts.capabilities.end(), capability) !=
        facts.capabilities.end();
}

bool mergeProviderFacts(
    std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts>& providers,
    const vdrsuite::agent::BackendAgentLocalProviderFacts& facts)
{
    auto existing = std::find_if(
        providers.begin(), providers.end(),
        [&](const auto& value) { return value.providerId == facts.providerId; });
    if (existing == providers.end())
    {
        providers.push_back(facts);
        return true;
    }
    if (existing->providerKind != facts.providerKind ||
        existing->providerInstanceEpoch != facts.providerInstanceEpoch ||
        existing->providerGeneration != facts.providerGeneration ||
        existing->capabilityRevision != facts.capabilityRevision ||
        existing->available != facts.available)
        return false;
    for (const std::string& capability : facts.capabilities)
        if (!hasCapability(*existing, capability.c_str()))
            existing->capabilities.push_back(capability);
    return true;
}

CommandAvailability availableCommands(
    const BackendAgentCommandClientConfig& config)
{
    CommandAvailability availability;
    std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts> timerProviders;
    std::vector<std::string> supportedTimerTypes;
    bool timerSnapshotCoherent = true;

    const auto discoverTimer =
        [&](auto* transport,
            const std::initializer_list<std::pair<const char*, const char*>>&
                requirements) {
            bool requested = false;
            for (const auto& requirement : requirements)
                requested = requested ||
                    hasCommandType(config, requirement.first);
            if (!requested || transport == nullptr) return;

            vdrsuite::agent::BackendAgentLocalProviderFacts facts;
            std::string reason;
            try
            {
                if (!transport->discoverProvider(facts, reason))
                {
                    timerSnapshotCoherent = false;
                    return;
                }
            }
            catch (...)
            {
                timerSnapshotCoherent = false;
                return;
            }
            if (!vdrsuite::agent::backendAgentLocalProviderValidFacts(facts) ||
                facts.providerId !=
                    vdrsuite::agent::kBackendAgentNativeTimerCreateProviderId ||
                facts.providerKind !=
                    vdrsuite::agent::kBackendAgentNativeTimerCreateProviderKind)
            {
                timerSnapshotCoherent = false;
                return;
            }
            if (!facts.available) return;

            std::vector<std::string> supported;
            for (const auto& requirement : requirements)
                if (hasCommandType(config, requirement.first) &&
                    hasCapability(facts, requirement.second))
                    supported.push_back(requirement.first);
            if (supported.empty())
            {
                timerSnapshotCoherent = false;
                return;
            }
            if (!mergeProviderFacts(timerProviders, facts))
            {
                timerSnapshotCoherent = false;
                return;
            }
            supportedTimerTypes.insert(
                supportedTimerTypes.end(), supported.begin(), supported.end());
        };

    discoverTimer(
        config.nativeTimerCreateTransport,
        {{vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType,
          vdrsuite::agent::kBackendAgentNativeTimerCreateCapability}});
    discoverTimer(
        config.nativeTimerDeleteTransport,
        {{vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType,
          vdrsuite::agent::kBackendAgentNativeTimerDeleteCapability}});
    discoverTimer(
        config.nativeTimerModifyTransport,
        {{vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType,
          vdrsuite::agent::kBackendAgentNativeTimerUpdateCapability},
         {vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType,
          vdrsuite::agent::kBackendAgentNativeTimerToggleCapability}});

    bool recordingMarksModifyAvailable = false;
    if (hasCommandType(
            config,
            vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType) &&
        RecordingMarksModifyTransport != nullptr)
    {
        vdrsuite::agent::BackendAgentLocalProviderFacts facts;
        std::string reason;
        try
        {
            recordingMarksModifyAvailable =
                RecordingMarksModifyTransport->discoverProvider(facts, reason) &&
                facts.available &&
                vdrsuite::agent::backendAgentLocalProviderValidFacts(facts) &&
                facts.providerId ==
                    vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderId &&
                facts.providerKind ==
                    vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderKind &&
                hasCapability(
                    facts,
                    vdrsuite::agent::kBackendAgentRecordingMarksModifyCapability) &&
                mergeProviderFacts(availability.localProviders, facts);
        }
        catch (...)
        {
            recordingMarksModifyAvailable = false;
        }
    }

    bool recordingCutAvailable = false;
    if (hasCommandType(
            config,
            vdrsuite::agent::kBackendAgentRecordingCutCommandType) &&
        config.recordingCutTransport != nullptr)
    {
        vdrsuite::agent::BackendAgentLocalProviderFacts facts;
        std::string reason;
        try
        {
            recordingCutAvailable =
                config.recordingCutTransport->discoverProvider(facts, reason) &&
                facts.available &&
                vdrsuite::agent::backendAgentLocalProviderValidFacts(facts) &&
                facts.providerId ==
                    vdrsuite::agent::kBackendAgentRecordingCutProviderId &&
                facts.providerKind ==
                    vdrsuite::agent::kBackendAgentRecordingCutProviderKind &&
                hasCapability(
                    facts,
                    vdrsuite::agent::kBackendAgentRecordingCutCapability) &&
                mergeProviderFacts(availability.localProviders, facts);
        }
        catch (...)
        {
            recordingCutAvailable = false;
        }
    }

    bool legacyOsdInputAvailable = false;
    if (hasCommandType(config, kLegacyOsdInputCommandType) &&
        config.legacyOsdInputTransport != nullptr)
    {
        try
        {
            legacyOsdInputAvailable =
                config.legacyOsdInputTransport->legacyOsdInputAvailable();
        }
        catch (...)
        {
            legacyOsdInputAvailable = false;
        }
    }

    for (const std::string& type : config.commandTypes)
    {
        const bool timerType =
            type == vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||
            type == vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType;
        if (timerType)
        {
            if (timerSnapshotCoherent &&
                std::find(
                    supportedTimerTypes.begin(),
                    supportedTimerTypes.end(),
                    type) != supportedTimerTypes.end())
                availability.commandTypes.push_back(type);
            continue;
        }
        if (type == vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType)
        {
            if (recordingMarksModifyAvailable)
                availability.commandTypes.push_back(type);
            continue;
        }
        if (type == vdrsuite::agent::kBackendAgentRecordingCutCommandType)
        {
            if (recordingCutAvailable)
                availability.commandTypes.push_back(type);
            continue;
        }
        if (type == kLegacyOsdInputCommandType)
        {
            if (legacyOsdInputAvailable)
                availability.commandTypes.push_back(type);
            continue;
        }
        if (type == vdrsuite::mediatools::kMediaImportCommandType ||
            type == vdrsuite::mediatools::kMediaImportReadbackCommandType)
        {
            // Availability never comes from the module manifest or config alone.
            if (config.mediaImportExecutor)
            {
                try
                {
                    if (config.mediaImportExecutor->available())
                        availability.commandTypes.push_back(type);
                }
                catch (...) {} // Lost trust removes advertisement.
            }
            continue;
        }
        if (type == vdrsuite::mediatools::kMediaShrinkStageCommandType ||
            type == vdrsuite::mediatools::kMediaShrinkPublishCommandType)
        {
            // The shipped Agent has no HEVC executor. Config alone is not
            // enough to publish either mutating command capability.
            if (config.mediaShrinkExecutor)
            {
                try
                {
                    if (config.mediaShrinkExecutor->available(type))
                        availability.commandTypes.push_back(type);
                }
                catch (...) {} // Fail closed on trust/provenance errors.
            }
            continue;
        }
        // An add-on manifest or COMMAND_TYPES value cannot self-advertise
        // an unregistered executor. Reserve both namespaces until the
        // reviewed Backend-Agent handler is actually connected.
        if (type.rfind("addon.", 0) == 0 ||
            type.rfind("addons.", 0) == 0)
            continue;
        if (type != "vdr.native.probe")
        {
            availability.commandTypes.push_back(type);
            continue;
        }
        vdrsuite::agent::BackendAgentLocalProviderFacts facts;
        std::string reason;
        if (!vdrsuite::agent::backendAgentNativeProbeCommandAvailability(
                config.nativeProbeTransport, facts, reason) ||
            !mergeProviderFacts(availability.localProviders, facts))
            continue;
        availability.commandTypes.push_back(type);
    }

    if (timerSnapshotCoherent &&
        std::any_of(
            availability.commandTypes.begin(),
            availability.commandTypes.end(),
            [](const std::string& type) {
                return type.rfind("vdr.timer.", 0) == 0;
            }))
        for (const auto& facts : timerProviders)
            if (!mergeProviderFacts(availability.localProviders, facts))
            {
                availability.commandTypes.erase(
                    std::remove_if(
                        availability.commandTypes.begin(),
                        availability.commandTypes.end(),
                        [](const std::string& type) {
                            return type.rfind("vdr.timer.", 0) == 0;
                        }),
                    availability.commandTypes.end());
                availability.localProviders.clear();
                break;
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
    const bool mediaImportCommand = state.assignment.commandType ==
        vdrsuite::mediatools::kMediaImportCommandType;
    const bool mediaImportReadback = state.assignment.commandType ==
        vdrsuite::mediatools::kMediaImportReadbackCommandType;
    const bool mediaShrinkCommand =
        state.assignment.commandType ==
            vdrsuite::mediatools::kMediaShrinkStageCommandType ||
        state.assignment.commandType ==
            vdrsuite::mediatools::kMediaShrinkPublishCommandType;
    const bool timerCreateCommand = state.assignment.commandType ==
        vdrsuite::agent::kBackendAgentNativeTimerCreateCommandType;
    const bool timerDeleteCommand = state.assignment.commandType ==
        vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType;
    const bool timerModifyCommand =
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerUpdateCommandType ||
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentNativeTimerToggleCommandType;
    const bool recordingMarksModifyCommand =
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType;
    const bool recordingCutCommand =
        state.assignment.commandType ==
            vdrsuite::agent::kBackendAgentRecordingCutCommandType;
    if (timerCreateCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerCreateLocalState(config, context, state, reason))
        return false;
    if (timerDeleteCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerDeleteLocalState(config, context, state, reason))
        return false;
    if (timerModifyCommand && state.stateExtensionPresent &&
        !reconcileNativeTimerModifyLocalState(config, context, state, reason))
        return false;
    if (recordingMarksModifyCommand && state.stateExtensionPresent &&
        !reconcileRecordingMarksModifyLocalState(config, context, state, reason))
        return false;
    if (recordingCutCommand && state.stateExtensionPresent &&
        !reconcileRecordingCutLocalState(config, context, state, reason))
        return false;
    if (!sameContext(state.assignment, context))
    {
        if (state.resultPresent && state.receiptAcknowledged &&
            state.resultAcknowledged)
            return retireProtectedState(config.statePath, reason);

        // The native UPDATE/TOGGLE execution is already over. A durable
        // outcome_unknown result must reach the command's original ledger
        // even if the Agent re-enrolled into a newer backend generation.
        // This path sends evidence only: it NEVER resumes native execution,
        // changes the recorded assignment identity, or replays the command.
        // The control plane verifies the exact original assignment/evidence.
        const bool historicalModifyResult =
            timerModifyCommand &&
            state.receiptAcknowledged &&
            state.resultPresent && !state.resultAcknowledged &&
            state.assignment.backendId == context.backendId &&
            state.assignment.agentId == context.agentId &&
            state.assignment.backendGeneration < context.backendGeneration &&
            state.result.resultCategory == "outcome_unknown" &&
            state.result.verificationState == "outcome_unknown" &&
            state.result.retryClassification == "reconcile_only" &&
            !state.result.resultEvidence.empty();
        if (historicalModifyResult)
        {
            if (!sendResult(config, context, transport, state, reason))
                return false;
            reason = "native_modify_historical_result_reconciled";
            return true;
        }

        reason = "local_command_generation_fenced";
        return false;
    }

    if (mediaImportReadback && !state.resultPresent)
    {
        // Core releases this existing durable command ONLY after the
        // canonical operation becomes succeeded from recording-cache readback.
        // Never re-run the original media import, even on restart or replay.
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        std::string source, originalCommand, recordingProof, sourceProof;
        if (!vdrsuite::mediatools::parseMediaImportReadbackPayload(
                state.assignment.payload, source, originalCommand,
                recordingProof, sourceProof) ||
            state.assignment.payloadVersion !=
                (sourceProof.empty() ? 1U : 2U))
        {
            reason = "media_import_readback_payload_invalid";
            return false;
        }
        if (sourceProof.empty()) {
            // Existing (pre-REC4.6) imports have no bound source identity.
            createResult(state, "effect_reported", "verified", "succeeded",
                         "none", "none",
                         "VDR readback notice durably received; no source cleanup");
            state.result.resultEvidence = "media-tools-cleanup:not-performed";
        } else {
            // A crash after durable "starting" leaves the filesystem
            // outcome uncertain. Do not invoke cleanup a second time.
            if (state.dispatchState == "starting") {
                createResult(state, "starting", "outcome_unknown",
                             "outcome_unknown", "executor_unknown",
                             "reconcile_only",
                             "Source cleanup interrupted; inspect source before retry");
                state.result.resultEvidence = "media-tools-cleanup:failed";
                if (!persist(config.statePath, state, reason)) return false;
                return sendResult(config, context, transport, state, reason);
            }
            if (state.dispatchState != "not_started") {
                reason = "media_import_readback_state_invalid";
                return false;
            }
            // The one-shot cleanup is fenced durably before any source unlink.
            state.dispatchState = "starting";
            if (!persist(config.statePath, state, reason)) return false;
            auto cleanup = vdrsuite::mediatools::MediaImportCleanupOutcome::notPerformed;
            try {
                if (config.mediaImportExecutor &&
                    config.mediaImportExecutor->available())
                    cleanup = config.mediaImportExecutor->cleanupConfirmedSource(
                        source, sourceProof);
            } catch (...) {
                cleanup = vdrsuite::mediatools::MediaImportCleanupOutcome::failed;
            }
            using Cleanup = vdrsuite::mediatools::MediaImportCleanupOutcome;
            switch (cleanup) {
                case Cleanup::removed:
                    createResult(state, "effect_reported", "verified",
                                 "succeeded", "none", "none",
                                 "Confirmed VDR import; source cleanup succeeded");
                    state.result.resultEvidence = "media-tools-cleanup:removed";
                    break;
                case Cleanup::alreadyAbsent:
                    createResult(state, "effect_reported", "verified",
                                 "succeeded", "none", "none",
                                 "Confirmed VDR import; source already absent");
                    state.result.resultEvidence = "media-tools-cleanup:already-absent";
                    break;
                case Cleanup::notPerformed:
                    createResult(state, "effect_reported", "not_required",
                                 "succeeded", "none", "none",
                                 "Confirmed VDR import; cleanup not performed");
                    state.result.resultEvidence = "media-tools-cleanup:not-performed";
                    break;
                case Cleanup::failed:
                    createResult(state, "starting", "outcome_unknown",
                                 "rejected", "executor_unknown", "none",
                                 "Confirmed VDR import; source cleanup failed");
                    state.result.resultEvidence = "media-tools-cleanup:failed";
                    break;
            }
        }
        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
    }

    if (mediaImportCommand && !state.resultPresent)
    {
        // Same durable Agent inbox, receipt and result. Never re-execute after
        // a crash that could have occurred during non-idempotent remux.
        if (state.dispatchState != "not_started")
        {
            createResult(state, state.dispatchState, "outcome_unknown",
                         "outcome_unknown", "executor_unknown", "reconcile_only",
                         "Media import recovered after start; reconcile recording");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (state.assignment.deadline <= nowSeconds())
        {
            createResult(state, "not_started", "outcome_unknown", "rejected",
                         "expired", "none", "Media import deadline expired");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }
        bool authorized = false;
        try
        {
            authorized = config.mediaImportExecutor &&
                config.mediaImportExecutor->available() &&
                config.mediaImportExecutor->authorize(
                    state.assignment, context.backendId, context.backendGeneration);
        }
        catch (...) { authorized = false; }
        if (!authorized)
        {
            createResult(state, "not_started", "outcome_unknown", "rejected",
                         "fenced", "none", "Media import trust or grant unavailable");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }

        // The crash fence is durably written before any external side effect.
        state.dispatchState = "starting";
        if (!persist(config.statePath, state, reason)) return false;
        auto outcome = vdrsuite::mediatools::MediaImportAgentOutcome::unknown;
        try { outcome = config.mediaImportExecutor->execute(state.assignment); }
        catch (...) {} // Side effect unknown; do not retry.
        if (outcome == vdrsuite::mediatools::MediaImportAgentOutcome::rejected)
            createResult(state, "starting", "outcome_unknown", "rejected",
                         "fenced", "none", "Media import rejected without effect");
        else if (outcome == vdrsuite::mediatools::MediaImportAgentOutcome::marked)
        {
            createResult(state, "accepted_by_executor", "verified", "succeeded",
                         "none", "none", "Media Tools source marker read back");
            try {
                state.result.resultEvidence =
                    config.mediaImportExecutor->takeMarkerEvidence();
            } catch (...) { state.result.resultEvidence.clear(); }
            std::string sourceId;
            vdrsuite::mediatools::MediaImportDecision decision;
            if (!vdrsuite::mediatools::parseMediaImportSelection(
                    state.assignment.payload, sourceId, decision) ||
                state.result.resultEvidence !=
                    vdrsuite::mediatools::mediaImportMarkerEvidence(sourceId, decision))
            {
                createResult(state, "starting", "outcome_unknown",
                             "outcome_unknown", "executor_unknown",
                             "reconcile_only", "Marker readback proof unavailable");
                state.result.resultEvidence.clear();
            }
        }
        else
            createResult(
                state,
                outcome == vdrsuite::mediatools::MediaImportAgentOutcome::submitted
                    ? "accepted_by_executor" : "starting",
                "outcome_unknown", "outcome_unknown",
                outcome == vdrsuite::mediatools::MediaImportAgentOutcome::submitted
                    ? "none" : "executor_unknown",
                "reconcile_only", "Media import requires recording reconciliation");
        if (outcome == vdrsuite::mediatools::MediaImportAgentOutcome::submitted)
        {
            try
            {
                const auto evidence =
                    config.mediaImportExecutor->takeRecordingEvidence();
                std::string recordingProof, sourceProof;
                if (vdrsuite::mediatools::parseMediaImportResultEvidence(
                        evidence, recordingProof, sourceProof))
                    state.result.resultEvidence = evidence;
            }
            catch (...) {} // No readback proof, never report verification.
        }
        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
    }

    if (mediaShrinkCommand && !state.resultPresent)
    {
        // A crash after the durable start fence may have left a candidate
        // or a published recording. Never attempt this command a second time.
        if (state.dispatchState != "not_started")
        {
            createResult(state, state.dispatchState, "outcome_unknown",
                         "outcome_unknown", "executor_unknown", "reconcile_only",
                         "HEVC worker recovered after start; reconcile only");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (state.assignment.deadline <= nowSeconds())
        {
            createResult(state, "not_started", "outcome_unknown", "rejected",
                         "expired", "none", "HEVC command deadline expired");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }
        bool authorized = false;
        try
        {
            authorized = config.mediaShrinkExecutor &&
                config.mediaShrinkExecutor->available(state.assignment.commandType) &&
                config.mediaShrinkExecutor->authorize(
                    state.assignment, context.backendId, context.backendGeneration);
        }
        catch (...) { authorized = false; }
        if (!authorized)
        {
            createResult(state, "not_started", "outcome_unknown", "rejected",
                         "fenced", "none", "HEVC trust or grant unavailable");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }

        state.dispatchState = "starting";
        if (!persist(config.statePath, state, reason)) return false;
        auto outcome = vdrsuite::mediatools::MediaShrinkAgentOutcome::unknown;
        try { outcome = config.mediaShrinkExecutor->execute(state.assignment); }
        catch (...) {} // Unknown side effect; no blind retry.
        if (outcome == vdrsuite::mediatools::MediaShrinkAgentOutcome::rejected)
        {
            createResult(state, "starting", "outcome_unknown", "rejected",
                         "fenced", "none", "HEVC rejected without effect");
        }
        else
        {
            std::string evidence;
            if (outcome == vdrsuite::mediatools::MediaShrinkAgentOutcome::submitted)
            {
                try { evidence = config.mediaShrinkExecutor->takeReviewId(); }
                catch (...) {}
            }
            const bool reviewBound = vdrsuite::mediatools::validShrinkReviewEvidence(
                state.assignment, evidence);
            createResult(state, reviewBound ? "accepted_by_executor" : "starting",
                         "outcome_unknown", "outcome_unknown",
                         reviewBound ? "none" : "executor_unknown",
                         "reconcile_only", "HEVC requires Core reconciliation");
            if (reviewBound)
                state.result.resultEvidence = evidence;
        }
        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
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

    if (recordingMarksModifyCommand &&
        !state.stateExtensionPresent && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            reason = "recording_marks_modify_fresh_starting_state_invalid";
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
                "expired", "none",
                "command deadline expired before recording marks dispatch");
            if (!persist(config.statePath, state, reason)) return false;
            if (!sendResult(config, context, transport, state, reason)) return false;
            reason = "command_result_reconciled";
            return true;
        }
        if (!prepareFreshRecordingMarksModifyLocalStarting(
                config, state, currentTime, reason))
            return false;
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (RecordingMarksModifyTransport == nullptr)
        {
            reason = "recording_marks_modify_local_starting_handoff_persisted";
            return true;
        }
        if (!executeFreshRecordingMarksModifyAndPersistOutcome(
                config, context, state, reason))
            return false;
        if (!sendResult(config, context, transport, state, reason))
            return false;
        reason = "recording_marks_modify_executor_outcome_reconciled";
        return true;
    }

    if (recordingCutCommand &&
        !state.stateExtensionPresent && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            reason = "recording_cut_fresh_starting_state_invalid";
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
                "expired", "none",
                "command deadline expired before recording cut dispatch");
            if (!persist(config.statePath, state, reason)) return false;
            if (!sendResult(config, context, transport, state, reason)) return false;
            reason = "command_result_reconciled";
            return true;
        }
        if (!prepareFreshRecordingCutLocalStarting(
                config, state, currentTime, reason))
            return false;
        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;
        if (config.recordingCutTransport == nullptr)
        {
            reason = "recording_cut_local_starting_handoff_persisted";
            return true;
        }
        if (!executeFreshRecordingCutAndPersistOutcome(
                config, context, state, reason))
            return false;
        if (!sendResult(config, context, transport, state, reason))
            return false;
        reason = "recording_cut_executor_outcome_reconciled";
        return true;
    }

    const bool legacyOsdInputCommand =
        state.assignment.commandType == kLegacyOsdInputCommandType;
    if (legacyOsdInputCommand && !state.resultPresent)
    {
        if (state.dispatchState != "not_started")
        {
            createResult(
                state, state.dispatchState, "outcome_unknown",
                "outcome_unknown", "executor_unknown", "reconcile_only",
                "Legacy OSD input recovered after dispatch boundary; not re-executed");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }

        const std::int64_t currentTime = nowSeconds();
        if (state.assignment.deadline <= currentTime)
        {
            createResult(
                state, "not_started", "not_required",
                "rejected", "expired", "none",
                "Legacy OSD input deadline expired before dispatch");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }

        if (!state.receiptAcknowledged &&
            !sendReceipt(config, context, transport, state, reason))
            return false;

        LegacyOsdInputCommand command;
        if (!legacyOsdInputCommandParse(state.assignment.payload, command))
        {
            createResult(
                state, "not_started", "not_required",
                "rejected", "unsupported", "none",
                "Legacy OSD input payload rejected locally");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }
        if (config.legacyOsdInputTransport == nullptr)
        {
            createResult(
                state, "not_started", "not_required",
                "rejected", "unsupported", "none",
                "Legacy OSD input transport unavailable");
            if (!persist(config.statePath, state, reason)) return false;
            return sendResult(config, context, transport, state, reason);
        }

        state.dispatchState = "starting";
        if (!persist(config.statePath, state, reason)) return false;

        const vdrsuite::agent::SuiteBridgeCommandReply reply =
            config.legacyOsdInputTransport->executeLegacyOsdInput(
                command, state.assignment.requestFingerprint);

        if (!reply.transportSucceeded())
        {
            createResult(
                state, "starting", "outcome_unknown",
                "outcome_unknown", "executor_unknown", "reconcile_only",
                "Legacy OSD input transport outcome unknown; not retried");
        }
        else if (reply.replyCode == 900)
        {
            createResult(
                state, "effect_reported", "not_required",
                "succeeded", "none", "none",
                "Legacy OSD input dispatched; domain mutation success not implied");
        }
        else if (reply.replyCode == 555 || reply.replyCode == 554)
        {
            createResult(
                state, "accepted_by_executor", "not_required",
                "rejected", "fenced", "none",
                "Legacy OSD input rejected by local fence");
        }
        else if (reply.replyCode == 501 || reply.replyCode == 504)
        {
            createResult(
                state, "accepted_by_executor", "not_required",
                "rejected", "unsupported", "none",
                "Legacy OSD input action or schema unsupported");
        }
        else
        {
            createResult(
                state, "accepted_by_executor", "outcome_unknown",
                "rejected", "executor_unknown", "reconcile_only",
                "Legacy OSD input rejected by native executor");
        }

        if (!persist(config.statePath, state, reason)) return false;
        return sendResult(config, context, transport, state, reason);
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

BackendAgentCommandAvailabilitySnapshot discoverBackendAgentCommandAvailability(
    const BackendAgentCommandClientConfig& config)
{
    return availableCommands(config);
}

bool pollBackendAgentCommandWithAvailability(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    const BackendAgentCommandAvailabilitySnapshot& availability,
    bool refreshCapabilities,
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
    request.refreshCapabilities = refreshCapabilities;
    if (refreshCapabilities)
    {
        request.supportedCommandTypes = availability.commandTypes;
        request.localProviders = availability.localProviders;
    }
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
        reason = availability.commandTypes.empty()
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

bool pollBackendAgentCommand(
    const BackendAgentCommandClientConfig& config,
    const BackendAgentCommandClientContext& context,
    IBackendAgentControlPlaneTransport& transport,
    std::string& reason)
{
    return pollBackendAgentCommandWithAvailability(
        config,
        context,
        transport,
        discoverBackendAgentCommandAvailability(config),
        true,
        reason);
}

void setBackendAgentNativeProbeTransport(
    vdrsuite::agent::IBackendAgentNativeProbeTransport* transport)
{
    vdrsuite::agent::backendAgentNativeProbeCommandSetDefaultTransport(transport);
}

void setBackendAgentRecordingMarksModifyTransport(
    vdrsuite::agent::IBackendAgentRecordingMarksModifyTransport* transport)
{
    RecordingMarksModifyTransport = transport;
}
