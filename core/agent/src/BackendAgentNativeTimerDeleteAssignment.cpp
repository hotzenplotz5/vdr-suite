#include "BackendAgentNativeTimerDeleteAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerDelete.h"
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

BackendAgentCommandAssignment readAssignment(sqlite3_stmt* statement)
{
    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.protocolVersion = text(statement, 0);
    assignment.requestId = text(statement, 1);
    assignment.correlationId = text(statement, 2);
    assignment.operationId = text(statement, 3);
    assignment.jobId = text(statement, 4);
    assignment.attemptId = text(statement, 5);
    assignment.claimEpoch = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 6));
    assignment.commandId = text(statement, 7);
    assignment.backendId = text(statement, 8);
    assignment.agentId = text(statement, 9);
    assignment.agentInstanceId = text(statement, 10);
    assignment.backendGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 11));
    assignment.commandType = text(statement, 12);
    assignment.payloadVersion = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 13));
    assignment.payload = text(statement, 14);
    assignment.requestFingerprint = text(statement, 15);
    assignment.verificationPolicy = text(statement, 16);
    assignment.assignedAt = sqlite3_column_int64(statement, 17);
    assignment.deadline = sqlite3_column_int64(statement, 18);
    return assignment;
}

constexpr const char* kAssignmentColumns =
    "protocol_version,request_id,correlation_id,operation_id,job_id,attempt_id,"
    "claim_epoch,command_id,backend_id,agent_id,agent_instance_id,"
    "backend_generation,command_type,payload_version,payload,request_fingerprint,"
    "verification_policy,assigned_at,deadline";

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

std::optional<BackendAgentCommandAssignment>
BackendAgentCommandRepository::findAssignmentForOperation(
    const std::string& backendId,
    const std::string& operationId,
    const std::string& commandType) const
{
    if (!backendAgentCommandSafeIdentifier(backendId) ||
        !backendAgentCommandSafeIdentifier(operationId) ||
        !backendAgentCommandSafeIdentifier(commandType))
        return std::nullopt;

    sqlite3_stmt* statement = nullptr;
    const std::string sql = std::string("SELECT ") + kAssignmentColumns +
        " FROM backend_agent_commands WHERE backend_id=? AND operation_id=? "
        "AND command_type=? ORDER BY assigned_at,command_id LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, operationId) ||
        !bindText(statement, 3, commandType))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<BackendAgentCommandAssignment> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
        result = readAssignment(statement);
    sqlite3_finalize(statement);
    return result;
}

std::optional<vdrsuite::agent::BackendAgentLocalProviderSelection>
BackendAgentCommandRepository::localProviderSelectionForCommand(
    const std::string& commandId) const
{
    using namespace vdrsuite::agent;
    if (!backendAgentCommandSafeIdentifier(commandId)) return std::nullopt;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT selection_identity,backend_id,authority_domain,provider_id,"
        "provider_kind,ownership_generation,provider_instance_epoch,"
        "provider_generation,capability_revision,required_capability "
        "FROM backend_agent_command_provider_selections WHERE command_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, commandId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }

    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        sqlite3_finalize(statement);
        return std::nullopt;
    }

    const std::string identity = text(statement, 0);
    BackendAgentLocalProviderSelection selection;
    selection.backendId = text(statement, 1);
    selection.authorityDomain = text(statement, 2);
    selection.providerId = text(statement, 3);
    selection.providerKind = text(statement, 4);
    selection.ownershipGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 5));
    selection.providerInstanceEpoch = text(statement, 6);
    selection.providerGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 7));
    selection.capabilityRevision = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 8));
    selection.requiredCapability = text(statement, 9);
    sqlite3_finalize(statement);

    if (!backendAgentLocalProviderValidSelection(selection) ||
        identity != backendAgentLocalProviderSelectionIdentity(selection))
        return std::nullopt;
    return selection;
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
