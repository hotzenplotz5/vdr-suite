#include "BackendAgentMediaImportAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"
#include "MediaImportOperationFence.h"
#include "MediaToolsAgentExecutor.h"
#include "MutationOperationRepository.h"

#include <algorithm>

bool BackendAgentCommandRepository::ensureMediaImportAssignmentSchema()
{
    // Reuse the existing command journal and agent-side durable state; no
    // secondary queue. This unique key is the crash/retry idempotency fence.
    return database_.execute(
        "CREATE UNIQUE INDEX IF NOT EXISTS "
        "idx_backend_agent_media_import_operation "
        "ON backend_agent_commands(backend_id,operation_id) "
        "WHERE command_type='addon.media.import';");
}

namespace vdrsuite::agent {
namespace {
using Result = BackendAgentMediaImportAssignmentResult;
Result deny(const std::string& code) { Result r; r.reasonCode = code; return r; }

bool exactExisting(
    const BackendAgentCommandAssignment& a,
    const vdrsuite::operations::MutationOperation& operation,
    const std::string& payload,
    const BackendAgentRecord& agent)
{
    return a.commandType == vdrsuite::mediatools::kMediaImportCommandType &&
        a.operationId == operation.operationId &&
        a.backendId == operation.backendId &&
        a.backendGeneration == operation.backendGeneration &&
        a.agentId == agent.agentId &&
        a.agentInstanceId == agent.agentInstanceId &&
        a.payload == payload &&
        a.payloadVersion == 1 &&
        a.verificationPolicy == "reconciliation_required" &&
        backendAgentCommandValidAssignment(a);
}
} // namespace

BackendAgentMediaImportAssignmentService::BackendAgentMediaImportAssignmentService(
    vdrsuite::operations::MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    vdrsuite::addons::MediaImportOperationFence& fence)
    : operations_(operations), commands_(commands), agents_(agents),
      fence_(fence)
{
}

BackendAgentMediaImportAssignmentResult
BackendAgentMediaImportAssignmentService::assign(
    const RequestSecurityContext& systemContext,
    const std::string& operationId, std::int64_t now)
{
    using State = vdrsuite::operations::MutationOperationState;
    if (!systemContext.authenticated() ||
        systemContext.actor.type != ActorType::System || now <= 0 ||
        !backendAgentCommandSafeIdentifier(operationId))
        return deny("media_import_assignment_system_authority_required");

    const auto found = operations_.findById(operationId);
    if (!found.ok()) return deny("media_import_operation_not_found");
    const auto& op = found.operation;
    if ((op.state != State::accepted && op.state != State::dispatching) ||
        op.actionFamily != "media.import" ||
        op.resourceType != "MediaImportSource" ||
        op.verificationPolicy !=
            vdrsuite::operations::MutationOperationVerificationPolicy::
                reconciliationRequired ||
        op.requestedAt > now || op.deadline <= now ||
        op.deadline == 0)
        return deny("media_import_operation_not_dispatchable");

    const auto payload = operations_.findPayloadByOperationId(op.operationId);
    std::string selected;
    if (!payload.ok() || payload.payload.payloadType != "addon.media.import" ||
        payload.payload.payloadVersion != 1 ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            payload.payload.payload, selected) ||
        selected != op.resourceId)
        return deny("media_import_operation_payload_invalid");

    const auto agent = agents_.findAgentForBackend(op.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->leaseExpiresAt < now || agent->agentInstanceId.empty() ||
        agent->backendGeneration != op.backendGeneration)
        return deny("media_import_active_agent_required");

    if (!commands_.ensureMediaImportAssignmentSchema())
        return deny("media_import_assignment_schema_failed");

    const auto previous = commands_.findAssignmentForOperation(
        op.backendId, op.operationId,
        vdrsuite::mediatools::kMediaImportCommandType);
    if (previous)
    {
        std::string gate;
        if (op.state != State::dispatching ||
            !exactExisting(*previous, op, payload.payload.payload, *agent) ||
            !fence_.checkCandidate(*previous, now, gate))
            return deny("media_import_assignment_replay_fenced");
        Result replay;
        replay.accepted = true;
        replay.replayed = true;
        replay.reasonCode = "media_import_assignment_replayed";
        replay.assignment = *previous;
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
    a.commandType = vdrsuite::mediatools::kMediaImportCommandType;
    a.payloadVersion = 1;
    a.payload = payload.payload.payload;
    a.verificationPolicy = "reconciliation_required";
    a.assignedAt = now;
    a.deadline = std::min(op.deadline, now + 600);
    a.requestFingerprint = backendAgentCommandFingerprint(a);
    std::string gate;
    if (!fence_.checkCandidate(a, now, gate))
        return deny(gate.empty() ? "media_import_assignment_fenced" : gate);

    // The operation CAS is authoritative. If we crash before command INSERT,
    // a later call can finish this same dispatching operation. If INSERT
    // succeeds, its unique index + exact replay prevent a duplicate job.
    if (op.state == State::accepted)
    {
        const auto marked = operations_.transition(
            op.operationId, op.operationRevision,
            State::accepted, State::dispatching, "", now);
        if (!marked.ok())
            return deny("media_import_operation_dispatch_conflict");
    }

    if (commands_.insertAssignment(a))
    {
        Result created;
        created.accepted = true;
        created.reasonCode = "media_import_assignment_reserved";
        created.assignment = a;
        return created;
    }
    const auto raced = commands_.findAssignmentForOperation(
        op.backendId, op.operationId,
        vdrsuite::mediatools::kMediaImportCommandType);
    if (raced && exactExisting(
            *raced, op, payload.payload.payload, *agent) &&
        fence_.checkCandidate(*raced, now, gate))
    {
        Result replay;
        replay.accepted = true;
        replay.replayed = true;
        replay.reasonCode = "media_import_assignment_race_replayed";
        replay.assignment = *raced;
        return replay;
    }
    return deny("media_import_assignment_persist_failed");
}

} // namespace vdrsuite::agent
