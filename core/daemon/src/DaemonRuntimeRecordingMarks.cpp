#include "DaemonRuntimeRecordingMarks.h"

#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "BackendRegistryService.h"
#include "RecordingMarksApiRuntime.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace
{
std::mutex recordingMarksReconciliationMutex;
std::condition_variable recordingMarksReconciliationCv;
bool recordingMarksReconciliationStopRequested = false;
std::thread recordingMarksReconciliationThread;
const std::vector<std::unique_ptr<BackendRuntimeContext>>*
    recordingMarksReconciliationRuntimeContexts = nullptr;
BackendAgentCommandRepository* recordingMarksReconciliationCommands = nullptr;

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
        [agents, commands](const RecordingMarksMutationRequest& request) {
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
