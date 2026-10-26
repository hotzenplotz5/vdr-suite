#include "BackendAgentMediaShrinkAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaShrinkCommandFence.h"
#include "MediaShrinkOperationFence.h"
#include "MutationOperationRepository.h"

#include <algorithm>
#include <cstdint>
#include <string>

bool BackendAgentCommandRepository::ensureMediaShrinkAssignmentSchema()
{
    // A reservation is unique across both stage and publish types for the
    // same Core operation, including restart or competing dispatchers.
    return database_.execute(
        "CREATE UNIQUE INDEX IF NOT EXISTS "
        "idx_backend_agent_media_shrink_operation "
        "ON backend_agent_commands(backend_id,operation_id) "
        "WHERE command_type IN("
        "'addon.media.shrink.stage','addon.media.shrink.publish');");
}

namespace vdrsuite::agent {
namespace {
using Result = BackendAgentMediaShrinkAssignmentResult;
using State = vdrsuite::operations::MutationOperationState;
using Policy = vdrsuite::operations::MutationOperationVerificationPolicy;
using Decision = vdrsuite::addons::MediaShrinkCommandFenceResult;

Result deny(const std::string& code) {
    Result result;
    result.reasonCode = code;
    return result;
}

bool same(const BackendAgentCommandAssignment& assignment,
          const vdrsuite::operations::MutationOperation& operation,
          const vdrsuite::operations::MutationOperationPayload& payload,
          const BackendAgentRecord& agent)
{
    return backendAgentCommandValidAssignment(assignment) &&
        assignment.operationId == operation.operationId &&
        assignment.backendId == operation.backendId &&
        assignment.backendGeneration == operation.backendGeneration &&
        assignment.agentId == agent.agentId &&
        assignment.agentInstanceId == agent.agentInstanceId &&
        assignment.commandType == payload.payloadType &&
        assignment.payloadVersion == 1 &&
        assignment.payload == payload.payload &&
        assignment.verificationPolicy ==
            (operation.actionFamily == "media.shrink.stage"
                ? "readback_required" : "reconciliation_required");
}
}

BackendAgentMediaShrinkAssignmentService::BackendAgentMediaShrinkAssignmentService(
    vdrsuite::operations::MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    vdrsuite::addons::MediaShrinkOperationFence& fence)
    : operations_(operations), commands_(commands),
      agents_(agents), fence_(fence)
{
}

BackendAgentMediaShrinkAssignmentResult
BackendAgentMediaShrinkAssignmentService::assign(
    const RequestSecurityContext& system, const std::string& operationId,
    std::int64_t now)
{
    if (!system.authenticated() || system.actor.type != ActorType::System ||
        now <= 0 || !backendAgentCommandSafeIdentifier(operationId))
        return deny("media_shrink_system_authority_required");
    const auto selected = operations_.findById(operationId);
    if (!selected.ok()) return deny("media_shrink_operation_not_found");
    const auto& op = selected.operation;
    const bool stage = op.actionFamily == "media.shrink.stage";
    const bool publish = op.actionFamily == "media.shrink.publish";
    if ((!stage && !publish) ||
        (op.state != State::accepted && op.state != State::dispatching) ||
        op.requestedAt > now || op.deadline <= now || op.deadline == 0 ||
        op.resourceType != (stage ? "Recording" : "MediaShrinkReview") ||
        op.verificationPolicy != (stage
            ? Policy::readbackRequired : Policy::reconciliationRequired) ||
        op.expectedResourceFingerprint.empty())
        return deny("media_shrink_operation_not_dispatchable");

    const auto p = operations_.findPayloadByOperationId(op.operationId);
    const std::string type = stage
        ? vdrsuite::mediatools::kMediaShrinkStageCommandType
        : vdrsuite::mediatools::kMediaShrinkPublishCommandType;
    vdrsuite::mediatools::MediaShrinkAgentSelection shape;
    if (!p.ok() || p.payload.payloadType != type ||
        p.payload.payloadVersion != 1 ||
        !vdrsuite::mediatools::parseMediaShrinkAgentSelection(
            type, p.payload.payload, shape) ||
        op.resourceId != (stage ? shape.recordingId : shape.reviewId) ||
        op.expectedRevision != (stage ? "source-v1" :
                                   std::to_string(shape.expectedReviewRevision)))
        return deny("media_shrink_payload_invalid");

    const auto agent = agents_.findAgentForBackend(op.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->agentId.empty() || agent->agentInstanceId.empty() ||
        agent->backendGeneration != op.backendGeneration ||
        agent->leaseExpiresAt < now)
        return deny("media_shrink_active_agent_required");
    std::uint64_t grantedGeneration = 0;
    std::string reason;
    if (!fence_.effectiveCapability(
            op.actorId, op.backendId, type, now,
            grantedGeneration, reason) ||
        grantedGeneration != op.backendGeneration)
        return deny(reason.empty() ? "media_shrink_permission_denied" : reason);

    if (!commands_.ensureMediaShrinkAssignmentSchema())
        return deny("media_shrink_assignment_schema_failed");
    const auto existing = commands_.findAssignmentForOperation(
        op.backendId, op.operationId, type);
    if (existing) {
        if (op.state != State::dispatching ||
            !same(*existing, op, p.payload, *agent) ||
            vdrsuite::addons::inspectMediaShrinkCommandFence(
                op, p.payload, *existing, now) != Decision::candidateOnly)
            return deny("media_shrink_assignment_replay_fenced");
        Result replay;
        replay.accepted = true;
        replay.replayed = true;
        replay.reasonCode = "media_shrink_assignment_replayed";
        replay.assignment = *existing;
        return replay;
    }

    BackendAgentCommandAssignment a;
    a.present = true;
    a.requestId = backendAgentGenerateOpaqueId("req_", 8);
    a.correlationId = a.requestId;
    a.operationId = op.operationId;
    a.jobId = backendAgentGenerateOpaqueId("job_", 12);
    a.attemptId = backendAgentGenerateOpaqueId("att_", 12);
    a.claimEpoch = 1;
    a.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
    a.backendId = op.backendId;
    a.agentId = agent->agentId;
    a.agentInstanceId = agent->agentInstanceId;
    a.backendGeneration = agent->backendGeneration;
    a.commandType = type;
    a.payloadVersion = 1;
    a.payload = p.payload.payload;
    a.verificationPolicy = stage ? "readback_required" :
                                    "reconciliation_required";
    a.assignedAt = now;
    a.deadline = op.deadline;
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    if (!backendAgentCommandValidAssignment(a))
        return deny("media_shrink_assignment_invalid");

    // Inspect the exact would-be dispatching Operation before CAS. A failed
    // preflight must not change the durable lifecycle.
    auto inspected = op;
    inspected.state = State::dispatching;
    if (vdrsuite::addons::inspectMediaShrinkCommandFence(
            inspected, p.payload, a, now) != Decision::candidateOnly)
        return deny("media_shrink_candidate_fenced");

    if (op.state == State::accepted) {
        const auto marked = operations_.transition(
            op.operationId, op.operationRevision,
            State::accepted, State::dispatching, "", now);
        if (!marked.ok()) return deny("media_shrink_operation_conflict");
    }

    if (commands_.insertAssignment(a)) {
        Result created;
        created.accepted = true;
        created.reasonCode = "media_shrink_assignment_reserved";
        created.assignment = a;
        return created;
    }
    const auto raced = commands_.findAssignmentForOperation(
        op.backendId, op.operationId, type);
    if (raced && same(*raced, inspected, p.payload, *agent) &&
        vdrsuite::addons::inspectMediaShrinkCommandFence(
            inspected, p.payload, *raced, now) == Decision::candidateOnly) {
        Result replay;
        replay.accepted = true;
        replay.replayed = true;
        replay.reasonCode = "media_shrink_assignment_race_replayed";
        replay.assignment = *raced;
        return replay;
    }
    return deny("media_shrink_assignment_persist_failed");
}
} // namespace vdrsuite::agent
