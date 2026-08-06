#include "BackendAgentCommandDelivery.h"

#include "BackendAgentLifecycle.h"

#include <sstream>

std::optional<BackendAgentCommandAssignment>
BackendAgentCommandDeliveryService::assignNativeProbe(
    const RequestSecurityContext& context,
    const std::string& backendId,
    std::int64_t now,
    std::int64_t deadline,
    std::string& reason)
{
    if (!context.authenticated() || context.actor.type != ActorType::System ||
        !backendAgentCommandSafeIdentifier(backendId) || deadline <= now ||
        deadline - now > 3600)
    {
        reason = "invalid_native_probe_assignment_request";
        return std::nullopt;
    }
    const auto agent = agentRepository_.findAgentForBackend(backendId);
    if (!agent.has_value() || agent->revoked || agent->incompatible ||
        agent->agentInstanceId.empty() || agent->backendGeneration == 0 ||
        agent->leaseExpiresAt < now)
    {
        reason = "active_agent_lease_required";
        return std::nullopt;
    }
    if (!commandRepository_.hasCapability(
            backendId, agent->agentId, agent->agentInstanceId,
            agent->backendGeneration, "vdr.native.probe"))
    {
        reason = "native_probe_capability_required";
        return std::nullopt;
    }

    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = backendAgentGenerateOpaqueId("req_", 8);
    assignment.correlationId = assignment.requestId;
    assignment.operationId = backendAgentGenerateOpaqueId("op_", 12);
    assignment.jobId = backendAgentGenerateOpaqueId("job_", 12);
    assignment.attemptId = backendAgentGenerateOpaqueId("att_", 12);
    assignment.claimEpoch = 1;
    assignment.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
    assignment.backendId = backendId;
    assignment.agentId = agent->agentId;
    assignment.agentInstanceId = agent->agentInstanceId;
    assignment.backendGeneration = agent->backendGeneration;
    assignment.commandType = "vdr.native.probe";
    assignment.payloadVersion = 1;
    const std::string probeNonce = backendAgentGenerateOpaqueId("pbn_", 12);
    assignment.payload =
        "{\"probeSchema\":1,\"probeNonce\":\"" + probeNonce + "\"}";
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = now;
    assignment.deadline = deadline;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);

    if (!backendAgentCommandValidAssignment(assignment) ||
        !appendEvent(
            context, "agent.command.assigned", backendId,
            assignment.operationId, "assign-native-probe-command", "allow",
            "side_effect_free_native_probe", "attempted", now) ||
        !commandRepository_.insertAssignment(assignment))
    {
        reason = "native_probe_assignment_persist_failed";
        return std::nullopt;
    }
    reason = "native_probe_assigned";
    return assignment;
}
