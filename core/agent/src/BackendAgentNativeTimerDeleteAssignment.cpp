#include "BackendAgentNativeTimerDeleteAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "Database.h"

#include <sqlite3.h>

#include <optional>
#include <string>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
               statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

std::string text(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value
        ? std::string(reinterpret_cast<const char*>(value))
        : std::string();
}

bool requestValid(
    const RequestSecurityContext& context,
    const vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    return context.authenticated() && context.actor.type == ActorType::System &&
        backendAgentCommandSafeIdentifier(request.operationId) &&
        backendAgentCommandSafeIdentifier(request.operationRevision) &&
        backendAgentCommandSafeIdentifier(request.nativeTimerBindingId) &&
        backendAgentCommandSafeIdentifier(request.expectedBindingRevision) &&
        vdrsuite::agent::backendAgentNativeTimerDeleteCanonicalFingerprintValid(
            request.expectedNativeTimerFingerprint) &&
        backendAgentCommandSafeIdentifier(request.timerAssignmentId) &&
        backendAgentCommandSafeIdentifier(request.backendId) &&
        request.backendGeneration > 0 &&
        backendAgentCommandSafeIdentifier(request.backendNativeTimerId) &&
        request.controlPlaneClaimedAt > 0 &&
        request.controlPlaneClaimedAt <= now && now > 0 && deadline > now &&
        deadline - now <= 3600;
}

vdrsuite::agent::BackendAgentNativeTimerDeleteCommand domainCommand(
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentNativeTimerDeletePayload& payload)
{
    using namespace vdrsuite::agent;
    BackendAgentNativeTimerDeleteCommand command;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = payload.operationRevision;
    command.nativeTimerBindingId = payload.nativeTimerBindingId;
    command.expectedBindingRevision = payload.expectedBindingRevision;
    command.expectedNativeTimerFingerprint = payload.expectedNativeTimerFingerprint;
    command.timerAssignmentId = payload.timerAssignmentId;
    command.backendNativeTimerId = payload.backendNativeTimerId;
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

bool requestMatches(
    const vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentRequest& request,
    const BackendAgentCommandAssignment& assignment,
    const vdrsuite::agent::BackendAgentNativeTimerDeletePayload& payload)
{
    return assignment.commandType ==
               vdrsuite::agent::kBackendAgentNativeTimerDeleteCommandType &&
        assignment.payloadVersion ==
            vdrsuite::agent::kBackendAgentNativeTimerDeletePayloadVersion &&
        assignment.verificationPolicy == "readback_required" &&
        assignment.operationId == request.operationId &&
        assignment.backendId == request.backendId &&
        assignment.backendGeneration == request.backendGeneration &&
        payload.operationRevision == request.operationRevision &&
        payload.nativeTimerBindingId == request.nativeTimerBindingId &&
        payload.expectedBindingRevision == request.expectedBindingRevision &&
        payload.expectedNativeTimerFingerprint ==
            vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
                request.expectedNativeTimerFingerprint) &&
        payload.timerAssignmentId == request.timerAssignmentId &&
        payload.backendNativeTimerId == request.backendNativeTimerId &&
        payload.controlPlaneClaimedAt == request.controlPlaneClaimedAt;
}

} // namespace

bool BackendAgentCommandRepository::ensureNativeTimerDeleteAssignmentSchema()
{
    auto transactionLease = database_.acquireTransactionLease();
    (void)transactionLease;
    if (!database_.execute("BEGIN IMMEDIATE;")) return false;
    const bool ok = database_.execute(
            "CREATE UNIQUE INDEX IF NOT EXISTS "
            "idx_backend_agent_timer_delete_operation ON "
            "backend_agent_commands(backend_id,operation_id) "
            "WHERE command_type='vdr.timer.delete';") &&
        database_.execute(
            "DELETE FROM backend_agent_command_capabilities "
            "WHERE command_type='vdr.timer.delete';") &&
        database_.execute(
            "CREATE TRIGGER IF NOT EXISTS "
            "trg_backend_agent_timer_delete_dormant_capability "
            "BEFORE INSERT ON backend_agent_command_capabilities "
            "WHEN NEW.command_type='vdr.timer.delete' "
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
namespace
{
BackendAgentNativeTimerDeleteAssignmentResult rejected(
    const std::string& reasonCode)
{
    BackendAgentNativeTimerDeleteAssignmentResult result;
    result.reasonCode = reasonCode;
    return result;
}

bool exactExistingAssignment(
    BackendAgentCommandRepository& commandRepository,
    const BackendAgentRecord& agent,
    const BackendAgentNativeTimerDeleteAssignmentRequest& request,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode)
{
    BackendAgentNativeTimerDeletePayload payload;
    if (!backendAgentNativeTimerDeleteParsePayload(
            assignment.payload, payload, reasonCode) ||
        !requestMatches(request, assignment, payload) ||
        !backendAgentCommandValidAssignment(assignment))
    {
        reasonCode = "native_timer_delete_assignment_conflict";
        return false;
    }
    if (assignment.agentId != agent.agentId ||
        assignment.agentInstanceId != agent.agentInstanceId ||
        assignment.backendGeneration != agent.backendGeneration)
    {
        reasonCode = "native_timer_delete_agent_fence_stale";
        return false;
    }

    const auto recorded =
        commandRepository.localProviderSelectionForCommand(assignment.commandId);
    if (!recorded.has_value() ||
        !backendAgentLocalProviderSameFence(
            *recorded, payload.localProviderSelection))
    {
        reasonCode = "native_timer_delete_provider_selection_missing";
        return false;
    }

    const auto current = commandRepository.selectLocalProvider(
        request.backendId,
        agent.agentId,
        agent.agentInstanceId,
        agent.backendGeneration,
        kBackendAgentNativeTimerDeleteAuthorityDomain,
        kBackendAgentNativeTimerDeleteCapability,
        reasonCode);
    if (!current.has_value()) return false;
    if (!backendAgentLocalProviderSameFence(*recorded, *current))
    {
        reasonCode = "native_timer_delete_provider_selection_stale";
        return false;
    }

    const auto command = domainCommand(assignment, payload);
    if (!backendAgentNativeTimerDeleteValidCommand(command, reasonCode))
    {
        reasonCode = "native_timer_delete_assignment_contract_invalid";
        return false;
    }
    return true;
}
}

BackendAgentNativeTimerDeleteAssignmentService::
BackendAgentNativeTimerDeleteAssignmentService(
    BackendAgentCommandRepository& commandRepository,
    BackendAgentRepository& agentRepository)
    : commandRepository_(commandRepository), agentRepository_(agentRepository)
{
}

BackendAgentNativeTimerDeleteAssignmentResult
BackendAgentNativeTimerDeleteAssignmentService::assign(
    const RequestSecurityContext& context,
    const BackendAgentNativeTimerDeleteAssignmentRequest& request,
    std::int64_t now,
    std::int64_t deadline)
{
    if (!requestValid(context, request, now, deadline))
        return rejected("invalid_native_timer_delete_assignment_request");

    const auto agent = agentRepository_.findAgentForBackend(request.backendId);
    if (!agent.has_value() || agent->revoked || agent->incompatible ||
        agent->agentInstanceId.empty() || agent->backendGeneration == 0 ||
        agent->leaseExpiresAt < now)
        return rejected("active_agent_lease_required");
    if (agent->backendGeneration != request.backendGeneration)
        return rejected("native_timer_delete_backend_generation_conflict");

    if (!commandRepository_.ensureNativeTimerDeleteAssignmentSchema())
        return rejected("native_timer_delete_assignment_schema_failed");

    if (const auto existing = commandRepository_.findAssignmentForOperation(
            request.backendId,
            request.operationId,
            kBackendAgentNativeTimerDeleteCommandType);
        existing.has_value())
    {
        std::string reason;
        if (!exactExistingAssignment(
                commandRepository_, *agent, request, *existing, reason))
            return rejected(reason);
        BackendAgentNativeTimerDeleteAssignmentResult result;
        result.accepted = true;
        result.replayed = true;
        result.reasonCode = "native_timer_delete_assignment_replayed";
        result.assignment = *existing;
        return result;
    }

    std::string reason;
    const auto selection = commandRepository_.selectLocalProvider(
        request.backendId,
        agent->agentId,
        agent->agentInstanceId,
        agent->backendGeneration,
        kBackendAgentNativeTimerDeleteAuthorityDomain,
        kBackendAgentNativeTimerDeleteCapability,
        reason);
    if (!selection.has_value()) return rejected(reason);

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = request.operationRevision;
    payload.nativeTimerBindingId = request.nativeTimerBindingId;
    payload.expectedBindingRevision = request.expectedBindingRevision;
    payload.expectedNativeTimerFingerprint =
        backendAgentNativeTimerDeleteFingerprintToken(
            request.expectedNativeTimerFingerprint);
    payload.timerAssignmentId = request.timerAssignmentId;
    payload.backendNativeTimerId = request.backendNativeTimerId;
    payload.controlPlaneClaimedAt = request.controlPlaneClaimedAt;
    payload.localProviderSelection = *selection;

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
    assignment.commandType = kBackendAgentNativeTimerDeleteCommandType;
    assignment.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    assignment.payload = backendAgentNativeTimerDeletePayload(payload);
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = now;
    assignment.deadline = deadline;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);

    const auto command = domainCommand(assignment, payload);
    if (assignment.payload.empty() ||
        !backendAgentNativeTimerDeleteValidCommand(command, reason) ||
        !backendAgentCommandValidAssignment(assignment))
        return rejected("native_timer_delete_assignment_contract_invalid");

    if (commandRepository_.insertAssignment(assignment, &*selection))
    {
        BackendAgentNativeTimerDeleteAssignmentResult result;
        result.accepted = true;
        result.reasonCode = "native_timer_delete_assigned";
        result.assignment = assignment;
        return result;
    }

    const auto raced = commandRepository_.findAssignmentForOperation(
        request.backendId,
        request.operationId,
        kBackendAgentNativeTimerDeleteCommandType);
    if (raced.has_value() &&
        exactExistingAssignment(
            commandRepository_, *agent, request, *raced, reason))
    {
        BackendAgentNativeTimerDeleteAssignmentResult result;
        result.accepted = true;
        result.replayed = true;
        result.reasonCode = "native_timer_delete_assignment_replayed";
        result.assignment = *raced;
        return result;
    }

    return rejected(
        raced.has_value() ? reason : "native_timer_delete_assignment_persist_failed");
}

} // namespace vdrsuite::agent
