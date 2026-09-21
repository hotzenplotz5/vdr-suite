#include "BackendAgentRecordingMarksModifyAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentRecordingMarksModifyPayload.h"
#include "Database.h"

#include <optional>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

BackendAgentRecordingMarksModifyAssignmentResult rejected(
    const std::string& reasonCode)
{
    BackendAgentRecordingMarksModifyAssignmentResult result;
    result.reasonCode = reasonCode;
    return result;
}

bool requestValid(
    const RequestSecurityContext& context,
    const BackendAgentRecordingMarksModifyAssignmentRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    return context.authenticated() &&
        context.actor.type == ActorType::System &&
        backendAgentCommandSafeIdentifier(request.operationId) &&
        backendAgentCommandSafeIdentifier(request.operationRevision) &&
        backendAgentRecordingMarksModifyRevisionTokenValid(
            request.recordingKey) &&
        backendAgentRecordingMarksModifyRevisionTokenValid(
            request.expectedMarksRevision) &&
        backendAgentRecordingMarksModifyFrameShapeValid(
            request.kind,
            request.sourceFrame,
            request.targetFrame,
            request.replacementFrames) &&
        backendAgentCommandSafeIdentifier(request.backendId) &&
        request.backendGeneration > 0 &&
        request.controlPlaneClaimedAt > 0 &&
        request.controlPlaneClaimedAt <= now &&
        now > 0 &&
        deadline > now &&
        deadline - now <= 3600;
}

BackendAgentRecordingMarksModifyPayload payloadFor(
    const BackendAgentRecordingMarksModifyAssignmentRequest& request,
    const BackendAgentLocalProviderSelection& selection)
{
    BackendAgentRecordingMarksModifyPayload payload;
    payload.kind = request.kind;
    payload.operationRevision = request.operationRevision;
    payload.recordingKey = request.recordingKey;
    payload.expectedMarksRevision = request.expectedMarksRevision;
    payload.sourceFrame = request.sourceFrame;
    payload.targetFrame = request.targetFrame;
    payload.replacementFrames = request.replacementFrames;
    payload.backendId = request.backendId;
    payload.backendGeneration = request.backendGeneration;
    payload.controlPlaneClaimedAt = request.controlPlaneClaimedAt;
    payload.localProviderSelection = selection;
    return payload;
}

BackendAgentRecordingMarksModifyCommand domainCommand(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentRecordingMarksModifyPayload& payload)
{
    BackendAgentRecordingMarksModifyCommand command;
    command.kind = payload.kind;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = payload.operationRevision;
    command.recordingKey = payload.recordingKey;
    command.expectedMarksRevision = payload.expectedMarksRevision;
    command.sourceFrame = payload.sourceFrame;
    command.targetFrame = payload.targetFrame;
    command.replacementFrames = payload.replacementFrames;
    command.jobId = assignment.jobId;
    command.attemptId = assignment.attemptId;
    command.claimEpoch = assignment.claimEpoch;
    command.backendId = assignment.backendId;
    command.agentId = assignment.agentId;
    command.agentInstanceId = assignment.agentInstanceId;
    command.backendGeneration = assignment.backendGeneration;
    command.controlPlaneClaimedAt = payload.controlPlaneClaimedAt;
    command.localProviderSelection = payload.localProviderSelection;
    return command;
}

bool exactRequest(
    const BackendAgentRecordingMarksModifyAssignmentRequest& request,
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentRecordingMarksModifyPayload& payload)
{
    return assignment.commandType ==
            kBackendAgentRecordingMarksModifyCommandType &&
        assignment.payloadVersion ==
            kBackendAgentRecordingMarksModifyPayloadVersion &&
        assignment.verificationPolicy == "readback_required" &&
        assignment.operationId == request.operationId &&
        assignment.backendId == request.backendId &&
        assignment.backendGeneration == request.backendGeneration &&
        payload.kind == request.kind &&
        payload.operationRevision == request.operationRevision &&
        payload.recordingKey == request.recordingKey &&
        payload.expectedMarksRevision == request.expectedMarksRevision &&
        payload.sourceFrame == request.sourceFrame &&
        payload.targetFrame == request.targetFrame &&
        payload.replacementFrames == request.replacementFrames &&
        payload.backendId == request.backendId &&
        payload.backendGeneration == request.backendGeneration &&
        payload.controlPlaneClaimedAt == request.controlPlaneClaimedAt;
}

bool exactExisting(
    BackendAgentCommandRepository& repository,
    const BackendAgentRecord& agent,
    const BackendAgentRecordingMarksModifyAssignmentRequest& request,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    BackendAgentRecordingMarksModifyPayload payload;
    if (!backendAgentRecordingMarksModifyParsePayload(
            assignment.payload, payload, reasonCode) ||
        !exactRequest(request, assignment, payload) ||
        !backendAgentCommandValidAssignment(assignment))
    {
        reasonCode = "recording_marks_modify_assignment_conflict";
        return false;
    }

    if (assignment.agentId != agent.agentId ||
        assignment.agentInstanceId != agent.agentInstanceId ||
        assignment.backendGeneration != agent.backendGeneration)
    {
        reasonCode = "recording_marks_modify_agent_fence_stale";
        return false;
    }

    const auto recorded = repository.localProviderSelectionForCommand(
        assignment.commandId);
    if (!recorded.has_value() ||
        !backendAgentLocalProviderSameFence(
            *recorded, payload.localProviderSelection))
    {
        reasonCode = "recording_marks_modify_provider_selection_missing";
        return false;
    }

    const auto current = repository.selectLocalProvider(
        request.backendId,
        agent.agentId,
        agent.agentInstanceId,
        agent.backendGeneration,
        kBackendAgentRecordingMarksModifyAuthorityDomain,
        kBackendAgentRecordingMarksModifyCapability,
        reasonCode);
    if (!current.has_value() ||
        !backendAgentLocalProviderSameFence(*recorded, *current))
    {
        reasonCode = "recording_marks_modify_provider_selection_stale";
        return false;
    }

    if (!backendAgentRecordingMarksModifyValidCommand(
            domainCommand(assignment, payload), reasonCode))
    {
        reasonCode = "recording_marks_modify_assignment_contract_invalid";
        return false;
    }

    reasonCode.clear();
    return true;
}

}
} // namespace vdrsuite::agent

bool BackendAgentCommandRepository::ensureRecordingMarksModifyAssignmentSchema()
{
    auto lease = database_.acquireTransactionLease();
    (void)lease;
    if (!database_.execute("BEGIN IMMEDIATE;")) return false;

    const bool ok = database_.execute(
        "CREATE UNIQUE INDEX IF NOT EXISTS "
        "idx_backend_agent_recording_marks_modify_operation "
        "ON backend_agent_commands(backend_id,operation_id) "
        "WHERE command_type='vdr.recording.marks.modify';") &&
        database_.execute(
            "DELETE FROM backend_agent_command_capabilities "
            "WHERE command_type='vdr.recording.marks.modify';") &&
        database_.execute(
            "CREATE TRIGGER IF NOT EXISTS "
            "trg_backend_agent_recording_marks_modify_dormant_capability "
            "BEFORE INSERT ON backend_agent_command_capabilities "
            "WHEN NEW.command_type='vdr.recording.marks.modify' "
            "BEGIN SELECT RAISE(IGNORE); END;");

    if (!ok || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    return true;
}

namespace vdrsuite::agent
{

BackendAgentRecordingMarksModifyAssignmentService::
BackendAgentRecordingMarksModifyAssignmentService(
    BackendAgentCommandRepository& commandRepository,
    BackendAgentRepository& agentRepository)
    : commandRepository_(commandRepository),
      agentRepository_(agentRepository)
{
}

BackendAgentRecordingMarksModifyAssignmentResult
BackendAgentRecordingMarksModifyAssignmentService::assign(
    const RequestSecurityContext& context,
    const BackendAgentRecordingMarksModifyAssignmentRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    if (!requestValid(context, request, now, deadline))
        return rejected("invalid_recording_marks_modify_assignment_request");

    const auto agent = agentRepository_.findAgentForBackend(request.backendId);
    if (!agent.has_value() ||
        agent->revoked ||
        agent->incompatible ||
        agent->agentInstanceId.empty() ||
        agent->backendGeneration == 0 ||
        agent->leaseExpiresAt < now)
    {
        return rejected("active_agent_lease_required");
    }

    if (agent->backendGeneration != request.backendGeneration)
        return rejected("recording_marks_modify_backend_generation_conflict");

    if (!commandRepository_.ensureRecordingMarksModifyAssignmentSchema())
        return rejected("recording_marks_modify_assignment_schema_failed");

    if (const auto existing = commandRepository_.findAssignmentForOperation(
            request.backendId,
            request.operationId,
            kBackendAgentRecordingMarksModifyCommandType);
        existing.has_value())
    {
        std::string reasonCode;
        if (!exactExisting(
                commandRepository_, *agent, request, *existing, reasonCode))
        {
            return rejected(reasonCode);
        }

        BackendAgentRecordingMarksModifyAssignmentResult result;
        result.accepted = true;
        result.replayed = true;
        result.reasonCode = "recording_marks_modify_assignment_replayed";
        result.assignment = *existing;
        return result;
    }

    std::string reasonCode;
    const auto selection = commandRepository_.selectLocalProvider(
        request.backendId,
        agent->agentId,
        agent->agentInstanceId,
        agent->backendGeneration,
        kBackendAgentRecordingMarksModifyAuthorityDomain,
        kBackendAgentRecordingMarksModifyCapability,
        reasonCode);
    if (!selection.has_value()) return rejected(reasonCode);

    const auto payload = payloadFor(request, *selection);
    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = backendAgentGenerateOpaqueId("req_", 8);
    assignment.correlationId = assignment.requestId;
    assignment.operationId = request.operationId;
    assignment.jobId = backendAgentGenerateOpaqueId("job_", 12);
    assignment.attemptId = backendAgentGenerateOpaqueId("att_", 12);
    assignment.claimEpoch = 1;
    assignment.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
    assignment.backendId = request.backendId;
    assignment.agentId = agent->agentId;
    assignment.agentInstanceId = agent->agentInstanceId;
    assignment.backendGeneration = agent->backendGeneration;
    assignment.commandType = kBackendAgentRecordingMarksModifyCommandType;
    assignment.payloadVersion = kBackendAgentRecordingMarksModifyPayloadVersion;
    assignment.payload = backendAgentRecordingMarksModifyPayload(payload);
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = now;
    assignment.deadline = deadline;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);

    if (assignment.payload.empty() ||
        !backendAgentRecordingMarksModifyValidCommand(
            domainCommand(assignment, payload), reasonCode) ||
        !backendAgentCommandValidAssignment(assignment))
    {
        return rejected("recording_marks_modify_assignment_contract_invalid");
    }

    if (commandRepository_.insertAssignment(assignment, &*selection))
    {
        BackendAgentRecordingMarksModifyAssignmentResult result;
        result.accepted = true;
        result.reasonCode = "recording_marks_modify_assigned";
        result.assignment = assignment;
        return result;
    }

    const auto raced = commandRepository_.findAssignmentForOperation(
        request.backendId,
        request.operationId,
        kBackendAgentRecordingMarksModifyCommandType);
    if (raced.has_value() &&
        exactExisting(
            commandRepository_, *agent, request, *raced, reasonCode))
    {
        BackendAgentRecordingMarksModifyAssignmentResult result;
        result.accepted = true;
        result.replayed = true;
        result.reasonCode = "recording_marks_modify_assignment_replayed";
        result.assignment = *raced;
        return result;
    }

    return rejected(
        raced.has_value()
            ? reasonCode
            : "recording_marks_modify_assignment_persist_failed");
}

}
