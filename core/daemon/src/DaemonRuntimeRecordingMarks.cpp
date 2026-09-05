#include "DaemonRuntimeRecordingMarks.h"

#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyAssignment.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "BackendRegistryService.h"
#include "RecordingMarksApiRuntime.h"

#include <chrono>
#include <string>

namespace
{
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

    return RecordingMarksApiRuntime::instance().configure(
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
            assignmentRequest.controlPlaneClaimedAt = now;

            const auto existing = commands->findAssignmentForOperation(
                request.backendId,
                request.operationId,
                vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType);
            if (request.replayOnly && !existing.has_value())
            {
                dispatch.reasonCode =
                    "recording_marks_modify_assignment_not_found";
                return dispatch;
            }
            if (existing.has_value())
            {
                vdrsuite::agent::BackendAgentRecordingMarksModifyPayload
                    existingPayload;
                std::string reasonCode;
                if (!vdrsuite::agent::backendAgentRecordingMarksModifyParsePayload(
                        existing->payload,
                        existingPayload,
                        reasonCode))
                {
                    dispatch.reasonCode =
                        "recording_marks_modify_assignment_conflict";
                    return dispatch;
                }
                assignmentRequest.controlPlaneClaimedAt =
                    existingPayload.controlPlaneClaimedAt;
            }

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
}

void resetDaemonRecordingMarksRuntime()
{
    RecordingMarksApiRuntime::instance().reset();
}