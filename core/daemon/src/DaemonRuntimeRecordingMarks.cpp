#include "DaemonRuntimeRecordingMarks.h"

#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModify.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "BackendAgentRecordingMarksModifyTransport.h"
#include "BackendRegistryService.h"
#include "RecordingMarksApiRuntime.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::mutex recordingMarksReconciliationMutex;
std::condition_variable recordingMarksReconciliationCv;
bool recordingMarksReconciliationStopRequested = false;
std::thread recordingMarksReconciliationThread;
const std::vector<std::unique_ptr<BackendRuntimeContext>>*
    recordingMarksReconciliationRuntimeContexts = nullptr;
BackendAgentCommandRepository* recordingMarksReconciliationCommands = nullptr;

constexpr int RecordingMarksModifyCapabilityReplyCode = 900;
constexpr int RecordingMarksModifyStaleReplyCode = 555;
constexpr int RecordingMarksModifyRejectedReplyCode = 556;
constexpr int RecordingMarksModifyAcceptedReplyCode = 557;
constexpr int RecordingMarksModifyUnknownReplyCode = 558;
constexpr int RecordingMarksModifyReplayConflictReplyCode = 559;
constexpr int RecordingMarksModifyReplayLedgerFullReplyCode = 560;
constexpr const char* RecordingMarksModifyCapabilityProtocol =
    "vdr-suite-nmarks-cap/2";
constexpr const char* RecordingMarksModifyResultProtocol =
    "vdr-suite-nmarks-result/2";

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

vdrsuite::agent::BackendAgentRecordingMarksModifyKind mutationKind(
    RecordingMarksMutationKind kind)
{
    using AgentKind =
        vdrsuite::agent::BackendAgentRecordingMarksModifyKind;
    switch (kind)
    {
    case RecordingMarksMutationKind::Add: return AgentKind::add;
    case RecordingMarksMutationKind::Delete: return AgentKind::deleteMark;
    case RecordingMarksMutationKind::Move: return AgentKind::move;
    case RecordingMarksMutationKind::Reset: return AgentKind::reset;
    case RecordingMarksMutationKind::Replace: return AgentKind::replace;
    }
    return AgentKind::add;
}

std::vector<std::string> splitTokens(
    const std::string& input,
    std::size_t maximum)
{
    std::vector<std::string> values;
    std::size_t position = 0;
    while (position < input.size())
    {
        while (position < input.size() && input[position] == ' ') ++position;
        if (position == input.size()) break;
        const std::size_t end = input.find(' ', position);
        values.push_back(input.substr(
            position,
            end == std::string::npos ? std::string::npos : end - position));
        if (values.size() > maximum) return {};
        if (end == std::string::npos) break;
        position = end + 1;
    }
    return values;
}

std::string stableLocalIdentity(
    const std::string& prefix,
    const std::string& value)
{
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << prefix << std::hex << std::setw(16) << std::setfill('0') << hash;
    return output.str();
}

std::string localMutationIdentity(const RecordingMarksMutationRequest& request)
{
    std::ostringstream value;
    value << request.backendId << '|' << request.operationId << '|'
          << request.operationRevision << '|' << request.recordingKey << '|'
          << request.expectedMarksRevision << '|'
          << vdrsuite::agent::backendAgentRecordingMarksModifyKindName(
                 mutationKind(request.kind))
          << '|' << request.sourceFrame << '|' << request.targetFrame << '|';
    for (const int frame : request.replacementFrames) value << frame << ',';
    return value.str();
}

bool exactReplayRequest(
    const RecordingMarksMutationRequest& request,
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentRecordingMarksModifyPayload& payload)
{
    return backendAgentCommandValidAssignment(assignment) &&
        assignment.commandType ==
            vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType &&
        assignment.verificationPolicy == "readback_required" &&
        assignment.operationId == request.operationId &&
        assignment.backendId == request.backendId &&
        payload.kind == mutationKind(request.kind) &&
        payload.operationRevision == request.operationRevision &&
        payload.recordingKey == request.recordingKey &&
        payload.expectedMarksRevision == request.expectedMarksRevision &&
        payload.sourceFrame == request.sourceFrame &&
        payload.targetFrame == request.targetFrame &&
        payload.replacementFrames == request.replacementFrames &&
        payload.backendId == request.backendId &&
        payload.backendGeneration == assignment.backendGeneration;
}

RequestSecurityContext systemContext()
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_marks_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:recording-marks-runtime",
        ActorType::System,
        "Recording marks control-plane runtime",
        true};
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

SuiteBridgeRecordingMarksResolver* recordingMarksResolverForBackend(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    const std::string& backendId)
{
    for (const auto& backendRuntimeContext : runtimeContexts)
    {
        if (!backendRuntimeContext || backendRuntimeContext->backendId != backendId)
            continue;
        if (!backendRuntimeContext->suiteBridgeAgentRuntime)
            return nullptr;

        const auto health = backendRuntimeContext->suiteBridgeAgentRuntime->health();
        if (!health.running ||
            !health.observation.hasDiscovery ||
            !health.observation.discovery.capabilityAvailable("recording-marks"))
        {
            return nullptr;
        }
        return backendRuntimeContext->ensureRecordingMarksResolver();
    }
    return nullptr;
}

BackendRuntimeContext* localRecordingMarksRuntimeForBackend(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    const std::string& backendId)
{
    for (const auto& backendRuntimeContext : runtimeContexts)
    {
        if (backendRuntimeContext &&
            backendRuntimeContext->backendId == backendId &&
            backendRuntimeContext->suiteBridgeTransport)
        {
            return backendRuntimeContext.get();
        }
    }
    return nullptr;
}

bool localRecordingMarksSelection(
    BackendRuntimeContext& runtime,
    const RecordingMarksMutationRequest& request,
    vdrsuite::agent::BackendAgentLocalProviderSelection& selection,
    std::string& reasonCode)
{
    selection = {};
    const auto reply =
        runtime.suiteBridgeTransport->discoverRecordingMarksModifyContract();
    if (!reply.transportSucceeded())
    {
        reasonCode = "recording_marks_modify_suitebridge_capability_unavailable";
        return false;
    }

    const std::vector<std::string> values = splitTokens(reply.payload, 10);
    if (reply.replyCode != RecordingMarksModifyCapabilityReplyCode ||
        values.size() != 10 ||
        values[0] != RecordingMarksModifyCapabilityProtocol ||
        values[1] !=
            vdrsuite::agent::kBackendAgentRecordingMarksModifyCapability ||
        values[2] != "2" || values[3] != "recording-marks-modify" ||
        values[4] != "enabled" ||
        values[5] !=
            vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderKind ||
        values[6].empty() || values[7] != "1" || values[8] != "2" ||
        values[9] != "enabled")
    {
        reasonCode = "recording_marks_modify_suitebridge_capability_unavailable";
        return false;
    }

    selection.backendId = request.backendId;
    selection.authorityDomain =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyAuthorityDomain;
    selection.providerId =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderId;
    selection.providerKind =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyProviderKind;
    selection.ownershipGeneration = 1;
    selection.providerInstanceEpoch = values[6];
    selection.providerGeneration = 1;
    selection.capabilityRevision = 2;
    selection.requiredCapability =
        vdrsuite::agent::kBackendAgentRecordingMarksModifyCapability;
    if (!vdrsuite::agent::backendAgentLocalProviderValidSelection(selection))
    {
        selection = {};
        reasonCode = "recording_marks_modify_suitebridge_capability_invalid";
        return false;
    }
    reasonCode.clear();
    return true;
}

std::optional<RecordingMarksMutationDispatchResult>
dispatchLocalRecordingMarksMutation(
    const RecordingMarksMutationRequest& request,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts)
{
    BackendRuntimeContext* const runtime =
        localRecordingMarksRuntimeForBackend(runtimeContexts, request.backendId);
    if (runtime == nullptr) return std::nullopt;

    RecordingMarksMutationDispatchResult dispatch;
    vdrsuite::agent::BackendAgentLocalProviderSelection selection;
    if (!localRecordingMarksSelection(
            *runtime,
            request,
            selection,
            dispatch.reasonCode))
    {
        return dispatch;
    }

    const std::string identity = localMutationIdentity(request);
    vdrsuite::agent::BackendAgentRecordingMarksModifyCommand command;
    command.kind = mutationKind(request.kind);
    command.commandId = stableLocalIdentity("local-marks-", identity);
    command.requestFingerprint = stableLocalIdentity("fp1_", "fp|" + identity);
    command.operationId = request.operationId;
    command.operationRevision = request.operationRevision;
    command.recordingKey = request.recordingKey;
    command.expectedMarksRevision = request.expectedMarksRevision;
    command.sourceFrame = request.sourceFrame;
    command.targetFrame = request.targetFrame;
    command.replacementFrames = request.replacementFrames;
    command.jobId = stableLocalIdentity("local-job-", identity);
    command.attemptId = "local-attempt-1";
    command.claimEpoch = 1;
    command.backendId = request.backendId;
    command.agentId = "daemon-local-suitebridge";
    command.agentInstanceId = "daemon-local-suitebridge";
    command.backendGeneration = 1;
    command.controlPlaneClaimedAt = 1;
    command.localProviderSelection = selection;

    std::string commandReason;
    if (!vdrsuite::agent::backendAgentRecordingMarksModifyValidCommand(
            command,
            commandReason))
    {
        dispatch.reasonCode = "recording_marks_modify_local_request_invalid";
        return dispatch;
    }

    vdrsuite::agent::BackendAgentRecordingMarksModifyTransportRequest
        transportRequest;
    transportRequest.command = command;
    transportRequest.localStartingPersistedAt = 1;
    dispatch.commandId = command.commandId;
    dispatch.requestFingerprint = command.requestFingerprint;

    const auto reply = runtime->suiteBridgeTransport
        ->executeRecordingMarksModifyContract(transportRequest);
    if (!reply.transportSucceeded())
    {
        dispatch.accepted = true;
        dispatch.replayed = request.replayOnly;
        dispatch.reasonCode = "recording_marks_modify_outcome_unknown";
        return dispatch;
    }

    const std::vector<std::string> values = splitTokens(reply.payload, 11);
    const bool typed = values.size() == 11 &&
        values[0] == RecordingMarksModifyResultProtocol &&
        values[1] == command.commandId &&
        values[2] == command.requestFingerprint &&
        values[3] ==
            vdrsuite::agent::kBackendAgentRecordingMarksModifyCapability &&
        values[4] == "2" &&
        values[5] == selection.providerInstanceEpoch &&
        values[6] == "1" && values[7] == "2";
    if (!typed)
    {
        dispatch.accepted = true;
        dispatch.replayed = request.replayOnly;
        dispatch.reasonCode = "recording_marks_modify_outcome_unknown";
        return dispatch;
    }

    if (reply.replyCode == RecordingMarksModifyAcceptedReplyCode &&
        values[8] == "accepted_unverified")
    {
        dispatch.accepted = true;
        dispatch.replayed = request.replayOnly;
        dispatch.reasonCode = request.replayOnly
            ? "recording_marks_modify_local_replayed"
            : "recording_marks_modify_local_accepted";
        if (request.replayOnly)
        {
            SuiteBridgeRecordingMarksResolver* const resolver =
                runtime->ensureRecordingMarksResolver();
            if (resolver != nullptr)
            {
                const VdrRecordingNativeMarks nativeMarks =
                    resolver->resolve(request.recordingKey);
                if (nativeMarks.availability ==
                        VdrRecordingNativeMarksAvailability::Available &&
                    nativeMarks.found &&
                    nativeMarks.recordingKey == request.recordingKey &&
                    vdrsuite::agent::
                        backendAgentRecordingMarksModifyRevisionTokenValid(
                            nativeMarks.marksRevision) &&
                    nativeMarks.marksRevision != request.expectedMarksRevision)
                {
                    dispatch.verified = true;
                    dispatch.canonicalMarksRevision =
                        nativeMarks.marksRevision;
                    dispatch.reasonCode =
                        "recording_marks_modify_local_verified_replayed";
                }
            }
        }
        return dispatch;
    }

    if (reply.replyCode == RecordingMarksModifyUnknownReplyCode &&
        values[8] == "outcome_unknown")
    {
        dispatch.accepted = true;
        dispatch.replayed = request.replayOnly;
        dispatch.reasonCode = "recording_marks_modify_outcome_unknown";
        return dispatch;
    }

    if (values[8] == "rejected_without_effect")
    {
        if (reply.replyCode == RecordingMarksModifyStaleReplyCode)
            dispatch.reasonCode = "recording_marks_modify_stale";
        else if (reply.replyCode == RecordingMarksModifyReplayConflictReplyCode)
            dispatch.reasonCode = "recording_marks_modify_replay_conflict";
        else if (reply.replyCode == RecordingMarksModifyReplayLedgerFullReplyCode)
            dispatch.reasonCode = "recording_marks_modify_replay_ledger_full";
        else if (reply.replyCode == RecordingMarksModifyRejectedReplyCode)
            dispatch.reasonCode = "recording_marks_modify_rejected";
        else
            dispatch.reasonCode = "recording_marks_modify_rejected";
        return dispatch;
    }

    dispatch.accepted = true;
    dispatch.replayed = request.replayOnly;
    dispatch.reasonCode = "recording_marks_modify_outcome_unknown";
    return dispatch;
}

void reconcileRecordingMarksMutationsOnce(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    const auto candidates = commands.recordingMarksModifyReconciliationCandidates();
    for (const auto& candidate : candidates)
    {
        SuiteBridgeRecordingMarksResolver* const resolver =
            recordingMarksResolverForBackend(
                runtimeContexts,
                candidate.assignment.backendId);
        if (resolver == nullptr) continue;

        const VdrRecordingNativeMarks nativeMarks =
            resolver->resolve(candidate.recordingKey);
        if (nativeMarks.availability !=
                VdrRecordingNativeMarksAvailability::Available ||
            !nativeMarks.found ||
            nativeMarks.recordingKey != candidate.recordingKey ||
            !vdrsuite::agent::backendAgentRecordingMarksModifyRevisionTokenValid(
                nativeMarks.marksRevision) ||
            nativeMarks.marksRevision == candidate.expectedMarksRevision)
        {
            continue;
        }

        BackendAgentRecordingMarksModifyVerification verification;
        std::string reasonCode;
        commands.verifyRecordingMarksModifyReadback(
            candidate.assignment.commandId,
            candidate.assignment.requestFingerprint,
            candidate.recordingKey,
            candidate.expectedMarksRevision,
            nativeMarks.marksRevision,
            nowSeconds(),
            verification,
            reasonCode);
    }
}

void stopRecordingMarksReconciliation()
{
    {
        std::lock_guard<std::mutex> lock(recordingMarksReconciliationMutex);
        recordingMarksReconciliationStopRequested = true;
    }
    recordingMarksReconciliationCv.notify_all();
    if (recordingMarksReconciliationThread.joinable())
        recordingMarksReconciliationThread.join();

    std::lock_guard<std::mutex> lock(recordingMarksReconciliationMutex);
    recordingMarksReconciliationRuntimeContexts = nullptr;
    recordingMarksReconciliationCommands = nullptr;
}

void startRecordingMarksReconciliation(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    stopRecordingMarksReconciliation();
    {
        std::lock_guard<std::mutex> lock(recordingMarksReconciliationMutex);
        recordingMarksReconciliationStopRequested = false;
        recordingMarksReconciliationRuntimeContexts = &runtimeContexts;
        recordingMarksReconciliationCommands = &commands;
    }

    recordingMarksReconciliationThread = std::thread([]() {
        std::unique_lock<std::mutex> lock(recordingMarksReconciliationMutex);
        while (!recordingMarksReconciliationStopRequested)
        {
            const auto* runtimeContexts =
                recordingMarksReconciliationRuntimeContexts;
            BackendAgentCommandRepository* const commands =
                recordingMarksReconciliationCommands;
            lock.unlock();
            if (runtimeContexts != nullptr && commands != nullptr)
                reconcileRecordingMarksMutationsOnce(*runtimeContexts, *commands);
            lock.lock();

            recordingMarksReconciliationCv.wait_for(
                lock,
                std::chrono::seconds(1),
                []() { return recordingMarksReconciliationStopRequested; });
        }
    });
}
}

bool configureDaemonRecordingMarksRuntime(
    VdrRecordingCacheRepository& recordingCacheRepository,
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy,
    BackendAgentRepository& backendAgentRepository,
    BackendAgentCommandRepository& backendAgentCommandRepository)
{
    VdrRecordingCacheRepository* const recordingCache =
        &recordingCacheRepository;
    const auto* const runtimeContexts = &backendRuntimeContexts;
    BackendRegistryService* const registry = &backendRegistryService;
    BackendAccessPolicy* const accessPolicy = &backendAccessPolicy;
    BackendAgentRepository* const agents = &backendAgentRepository;
    BackendAgentCommandRepository* const commands =
        &backendAgentCommandRepository;

    if (!commands->ensureRecordingMarksModifyReconciliationSchema())
        return false;

    const bool configured = RecordingMarksApiRuntime::instance().configure(
        [recordingCache](const std::string& backendId) {
            return recordingCache->findAllForBackend(backendId);
        },
        [runtimeContexts](const std::string& backendId) {
            for (const auto& backendRuntimeContext : *runtimeContexts) {
                if (!backendRuntimeContext ||
                    backendRuntimeContext->backendId != backendId)
                {
                    continue;
                }

                if (!backendRuntimeContext->suiteBridgeAgentRuntime) {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                const auto health =
                    backendRuntimeContext->suiteBridgeAgentRuntime->health();
                if (!health.running ||
                    !health.observation.hasDiscovery ||
                    !health.observation.discovery.capabilityAvailable(
                        "recording-marks"))
                {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                SuiteBridgeRecordingMarksResolver* resolver =
                    backendRuntimeContext->ensureRecordingMarksResolver();
                if (resolver == nullptr) {
                    return RecordingMarksBackendAccess{
                        RecordingMarksBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                return RecordingMarksBackendAccess{
                    RecordingMarksBackendAvailability::Available,
                    resolver};
            }
            return RecordingMarksBackendAccess{
                RecordingMarksBackendAvailability::BackendNotFound,
                nullptr};
        },
        [registry, accessPolicy](const std::string& backendId) {
            const BackendAccessDecision decision =
                accessPolicy->canWriteToBackend(*registry, backendId);
            RecordingMarksBackendWriteAccess access;
            access.allowed = decision.allowed;
            if (decision.allowed)
            {
                access.statusCode = 200;
                access.reasonCode = "recording_marks_backend_write_allowed";
            }
            else if (!decision.backendFound)
            {
                access.statusCode = 404;
                access.reasonCode = "backend_not_found";
            }
            else if (decision.readOnly)
            {
                access.statusCode = 403;
                access.reasonCode = "backend_read_only";
            }
            else
            {
                access.statusCode = 409;
                access.reasonCode = "backend_write_unavailable";
            }
            return access;
        },
        [agents, commands, runtimeContexts](
            const RecordingMarksMutationRequest& request) {
            if (const auto local = dispatchLocalRecordingMarksMutation(
                    request,
                    *runtimeContexts))
            {
                return *local;
            }

            RecordingMarksMutationDispatchResult dispatch;
            const auto existing = commands->findAssignmentForOperation(
                request.backendId,
                request.operationId,
                vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType);
            vdrsuite::agent::BackendAgentRecordingMarksModifyPayload existingPayload;
            if (existing.has_value())
            {
                std::string reasonCode;
                if (!vdrsuite::agent::backendAgentRecordingMarksModifyParsePayload(
                        existing->payload, existingPayload, reasonCode) ||
                    !exactReplayRequest(request, *existing, existingPayload))
                {
                    dispatch.reasonCode =
                        "recording_marks_modify_assignment_conflict";
                    return dispatch;
                }

                const auto verification =
                    commands->recordingMarksModifyVerificationForOperation(
                        request.backendId, request.operationId);
                if (verification.present)
                {
                    if (verification.commandId != existing->commandId ||
                        verification.requestFingerprint !=
                            existing->requestFingerprint ||
                        verification.recordingKey != request.recordingKey ||
                        verification.expectedMarksRevision !=
                            request.expectedMarksRevision)
                    {
                        dispatch.reasonCode =
                            "recording_marks_modify_verification_conflict";
                        return dispatch;
                    }
                    dispatch.accepted = true;
                    dispatch.replayed = true;
                    dispatch.verified = true;
                    dispatch.reasonCode =
                        "recording_marks_modify_verified_replayed";
                    dispatch.commandId = existing->commandId;
                    dispatch.requestFingerprint = existing->requestFingerprint;
                    dispatch.canonicalMarksRevision =
                        verification.canonicalMarksRevision;
                    return dispatch;
                }

                if (commands->recordingMarksModifyRejectedForOperation(
                        request.backendId,
                        request.operationId,
                        existing->commandId,
                        existing->requestFingerprint))
                {
                    dispatch.reasonCode = "recording_marks_modify_rejected";
                    return dispatch;
                }
            }
            else if (request.replayOnly)
            {
                dispatch.reasonCode =
                    "recording_marks_modify_assignment_not_found";
                return dispatch;
            }

            const auto agent = agents->findAgentForBackend(request.backendId);
            if (!agent.has_value())
            {
                dispatch.reasonCode = "active_agent_lease_required";
                return dispatch;
            }

            const std::int64_t now = nowSeconds();
            vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentRequest
                assignmentRequest;
            assignmentRequest.kind = mutationKind(request.kind);
            assignmentRequest.operationId = request.operationId;
            assignmentRequest.operationRevision = request.operationRevision;
            assignmentRequest.recordingKey = request.recordingKey;
            assignmentRequest.expectedMarksRevision =
                request.expectedMarksRevision;
            assignmentRequest.sourceFrame = request.sourceFrame;
            assignmentRequest.targetFrame = request.targetFrame;
            assignmentRequest.replacementFrames = request.replacementFrames;
            assignmentRequest.backendId = request.backendId;
            assignmentRequest.backendGeneration = agent->backendGeneration;
            assignmentRequest.controlPlaneClaimedAt = existing.has_value()
                ? existingPayload.controlPlaneClaimedAt
                : now;

            vdrsuite::agent::BackendAgentRecordingMarksModifyAssignmentService
                assignmentService(*commands, *agents);
            const auto assigned = assignmentService.assign(
                systemContext(),
                assignmentRequest,
                now,
                now + 300);
            dispatch.accepted = assigned.accepted;
            dispatch.replayed = assigned.replayed;
            dispatch.reasonCode = assigned.reasonCode;
            if (request.replayOnly && assigned.accepted && !assigned.replayed)
            {
                dispatch.accepted = false;
                dispatch.reasonCode =
                    "recording_marks_modify_replay_probe_invalid";
                return dispatch;
            }
            if (assigned.accepted)
            {
                dispatch.commandId = assigned.assignment.commandId;
                dispatch.requestFingerprint =
                    assigned.assignment.requestFingerprint;
            }
            return dispatch;
        });

    if (!configured) return false;
    startRecordingMarksReconciliation(backendRuntimeContexts, backendAgentCommandRepository);
    return true;
}

void resetDaemonRecordingMarksRuntime()
{
    stopRecordingMarksReconciliation();
    RecordingMarksApiRuntime::instance().reset();
}
