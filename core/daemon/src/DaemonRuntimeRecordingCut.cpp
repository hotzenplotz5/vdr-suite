#include "DaemonRuntimeRecordingCut.h"

#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingCutAssignment.h"
#include "BackendAgentRecordingCutPayload.h"
#include "BackendRegistryService.h"
#include "DaemonRecordingCutReconciliation.h"
#include "RecordingCutApiRuntime.h"
#include "SuiteBridgeRecordingCutStateResolver.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace
{
std::mutex recordingCutReconciliationMutex;
std::condition_variable recordingCutReconciliationCv;
bool recordingCutReconciliationStopRequested = false;
std::thread recordingCutReconciliationThread;
const std::vector<std::unique_ptr<BackendRuntimeContext>>*
    recordingCutReconciliationRuntimeContexts = nullptr;
BackendAgentCommandRepository* recordingCutReconciliationCommands = nullptr;

std::int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool exactReplayRequest(
    const RecordingCutStartRequest& request,
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentRecordingCutPayload& payload)
{
    return backendAgentCommandValidAssignment(assignment) &&
        assignment.commandType ==
            vdrsuite::agent::kBackendAgentRecordingCutCommandType &&
        assignment.verificationPolicy == "readback_required" &&
        assignment.operationId == request.operationId &&
        assignment.backendId == request.backendId &&
        payload.operationRevision == request.operationRevision &&
        payload.recordingKey == request.recordingKey &&
        payload.expectedMarksRevision == request.expectedMarksRevision &&
        payload.backendId == request.backendId &&
        payload.backendGeneration == assignment.backendGeneration;
}

RequestSecurityContext systemContext()
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_cut_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        "system:recording-cut-runtime",
        ActorType::System,
        "Recording cut control-plane runtime",
        true};
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

SuiteBridgeRecordingCutStateResolver* recordingCutStateResolverForBackend(
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
        if (!health.running || !health.observation.hasDiscovery)
            return nullptr;

        return backendRuntimeContext->ensureRecordingCutStateResolver();
    }
    return nullptr;
}

void reconcileRecordingCutsOnce(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    const auto candidates = commands.recordingCutReconciliationCandidates();
    for (const auto& candidate : candidates)
    {
        SuiteBridgeRecordingCutStateResolver* const resolver =
            recordingCutStateResolverForBackend(
                runtimeContexts,
                candidate.assignment.backendId);
        if (resolver == nullptr) continue;

        const VdrRecordingNativeCutState state =
            resolver->resolve(candidate.recordingKey);
        if (!daemonRecordingCutResultMatches(
                candidate.recordingKey,
                candidate.editedRecordingKey,
                state))
        {
            continue;
        }

        BackendAgentRecordingCutVerification verification;
        std::string reasonCode;
        commands.verifyRecordingCutResult(
            candidate.assignment.commandId,
            candidate.assignment.requestFingerprint,
            candidate.recordingKey,
            candidate.expectedMarksRevision,
            candidate.editedRecordingKey,
            nowSeconds(),
            verification,
            reasonCode);
    }
}

void stopRecordingCutReconciliation()
{
    {
        std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
        recordingCutReconciliationStopRequested = true;
    }
    recordingCutReconciliationCv.notify_all();
    if (recordingCutReconciliationThread.joinable())
        recordingCutReconciliationThread.join();

    std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
    recordingCutReconciliationRuntimeContexts = nullptr;
    recordingCutReconciliationCommands = nullptr;
}

void startRecordingCutReconciliation(
    const std::vector<std::unique_ptr<BackendRuntimeContext>>& runtimeContexts,
    BackendAgentCommandRepository& commands)
{
    stopRecordingCutReconciliation();
    {
        std::lock_guard<std::mutex> lock(recordingCutReconciliationMutex);
        recordingCutReconciliationStopRequested = false;
        recordingCutReconciliationRuntimeContexts = &runtimeContexts;
        recordingCutReconciliationCommands = &commands;
    }

    recordingCutReconciliationThread = std::thread([]() {
        std::unique_lock<std::mutex> lock(recordingCutReconciliationMutex);
        while (!recordingCutReconciliationStopRequested)
        {
            const auto* runtimeContexts =
                recordingCutReconciliationRuntimeContexts;
            BackendAgentCommandRepository* const commands =
                recordingCutReconciliationCommands;
            lock.unlock();
            if (runtimeContexts != nullptr && commands != nullptr)
                reconcileRecordingCutsOnce(*runtimeContexts, *commands);
            lock.lock();

            recordingCutReconciliationCv.wait_for(
                lock,
                std::chrono::seconds(1),
                []() { return recordingCutReconciliationStopRequested; });
        }
    });
}
}

bool configureDaemonRecordingCutRuntime(
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

    if (!commands->ensureRecordingCutReconciliationSchema())
        return false;

    const bool configured = RecordingCutApiRuntime::instance().configure(
        [recordingCache](const std::string& backendId) {
            return recordingCache->findAllForBackend(backendId);
        },
        [runtimeContexts](const std::string& backendId) {
            for (const auto& backendRuntimeContext : *runtimeContexts)
            {
                if (!backendRuntimeContext ||
                    backendRuntimeContext->backendId != backendId)
                {
                    continue;
                }

                if (!backendRuntimeContext->suiteBridgeAgentRuntime)
                {
                    return RecordingCutBackendAccess{
                        RecordingCutBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                const auto health =
                    backendRuntimeContext->suiteBridgeAgentRuntime->health();
                if (!health.running || !health.observation.hasDiscovery)
                {
                    return RecordingCutBackendAccess{
                        RecordingCutBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                SuiteBridgeRecordingCutStateResolver* resolver =
                    backendRuntimeContext->ensureRecordingCutStateResolver();
                if (resolver == nullptr)
                {
                    return RecordingCutBackendAccess{
                        RecordingCutBackendAvailability::CapabilityUnavailable,
                        nullptr};
                }

                return RecordingCutBackendAccess{
                    RecordingCutBackendAvailability::Available,
                    resolver};
            }

            return RecordingCutBackendAccess{
                RecordingCutBackendAvailability::BackendNotFound,
                nullptr};
        },
        [registry, accessPolicy](const std::string& backendId) {
            const BackendAccessDecision decision =
                accessPolicy->canWriteToBackend(*registry, backendId);
            RecordingCutBackendWriteAccess access;
            access.allowed = decision.allowed;
            if (decision.allowed)
            {
                access.statusCode = 200;
                access.reasonCode = "recording_cut_backend_write_allowed";
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
        [agents, commands](const RecordingCutStartRequest& request) {
            RecordingCutDispatchResult dispatch;
            const auto existing = commands->findAssignmentForOperation(
                request.backendId,
                request.operationId,
                vdrsuite::agent::kBackendAgentRecordingCutCommandType);
            vdrsuite::agent::BackendAgentRecordingCutPayload existingPayload;
            if (existing.has_value())
            {
                std::string reasonCode;
                if (!vdrsuite::agent::backendAgentRecordingCutParsePayload(
                        existing->payload, existingPayload, reasonCode) ||
                    !exactReplayRequest(request, *existing, existingPayload))
                {
                    dispatch.reasonCode = "recording_cut_assignment_conflict";
                    return dispatch;
                }

                const auto verification =
                    commands->recordingCutVerificationForOperation(
                        request.backendId,
                        request.operationId);
                if (verification.present)
                {
                    if (verification.commandId != existing->commandId ||
                        verification.requestFingerprint !=
                            existing->requestFingerprint ||
                        verification.recordingKey != request.recordingKey ||
                        verification.expectedMarksRevision !=
                            request.expectedMarksRevision ||
                        !VdrRecordingNativeIdentity::isValidKey(
                            verification.editedRecordingKey))
                    {
                        dispatch.reasonCode =
                            "recording_cut_verification_conflict";
                        return dispatch;
                    }
                    dispatch.accepted = true;
                    dispatch.replayed = true;
                    dispatch.verified = true;
                    dispatch.reasonCode = "recording_cut_verified_replayed";
                    dispatch.commandId = existing->commandId;
                    dispatch.requestFingerprint = existing->requestFingerprint;
                    dispatch.editedRecordingKey =
                        verification.editedRecordingKey;
                    return dispatch;
                }
            }
            else if (request.replayOnly)
            {
                dispatch.reasonCode = "recording_cut_assignment_not_found";
                return dispatch;
            }

            const auto agent = agents->findAgentForBackend(request.backendId);
            if (!agent.has_value())
            {
                dispatch.reasonCode = "active_agent_lease_required";
                return dispatch;
            }

            const std::int64_t now = nowSeconds();
            vdrsuite::agent::BackendAgentRecordingCutAssignmentRequest
                assignmentRequest;
            assignmentRequest.operationId = request.operationId;
            assignmentRequest.operationRevision = request.operationRevision;
            assignmentRequest.recordingKey = request.recordingKey;
            assignmentRequest.expectedMarksRevision =
                request.expectedMarksRevision;
            assignmentRequest.backendId = request.backendId;
            assignmentRequest.backendGeneration = agent->backendGeneration;
            assignmentRequest.controlPlaneClaimedAt = existing.has_value()
                ? existingPayload.controlPlaneClaimedAt
                : now;

            vdrsuite::agent::BackendAgentRecordingCutAssignmentService
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
                dispatch.reasonCode = "recording_cut_replay_probe_invalid";
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
    startRecordingCutReconciliation(
        backendRuntimeContexts,
        backendAgentCommandRepository);
    return true;
}

void resetDaemonRecordingCutRuntime()
{
    stopRecordingCutReconciliation();
    RecordingCutApiRuntime::instance().reset();
}
