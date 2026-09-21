#include "LegacyOsdInputService.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "LegacyOsdSessionService.h"
#include "OsdControllerLeaseService.h"
#include "OsdViewerBindingService.h"

#include <chrono>
#include <utility>

namespace
{
std::int64_t defaultNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

LegacyOsdInputResult reject(const std::string& error)
{
    LegacyOsdInputResult result;
    result.error = error;
    result.category = "rejected";
    return result;
}
}

LegacyOsdInputService::LegacyOsdInputService(
    LegacyOsdSessionService& sessionService,
    OsdViewerBindingService& viewerService,
    OsdControllerLeaseService& controllerService,
    BackendAgentRepository& agentRepository,
    BackendAgentCommandRepository& commandRepository,
    NowProvider nowProvider)
    : sessionService_(sessionService),
      viewerService_(viewerService),
      controllerService_(controllerService),
      agentRepository_(agentRepository),
      commandRepository_(commandRepository),
      nowProvider_(nowProvider ? std::move(nowProvider) : NowProvider(defaultNow))
{
}

bool LegacyOsdInputService::sameSemanticCommand(
    const LegacyOsdInputCommand& left,
    const LegacyOsdInputCommand& right) const
{
    return left.inputCommandId == right.inputCommandId &&
        left.legacyOsdSessionId == right.legacyOsdSessionId &&
        left.sessionRevision == right.sessionRevision &&
        left.viewerBindingId == right.viewerBindingId &&
        left.controllerLeaseId == right.controllerLeaseId &&
        left.controllerLeaseEpoch == right.controllerLeaseEpoch &&
        left.leaseRevision == right.leaseRevision &&
        left.actorId == right.actorId &&
        left.clientInstanceId == right.clientInstanceId &&
        left.backendId == right.backendId &&
        left.backendGeneration == right.backendGeneration &&
        left.osdSurfaceId == right.osdSurfaceId &&
        left.osdEpoch == right.osdEpoch &&
        left.action == right.action &&
        left.inputMode == right.inputMode &&
        left.repeatCount == right.repeatCount &&
        left.deadline == right.deadline;
}

bool LegacyOsdInputService::authorityCurrent(
    const LegacyOsdInputCommand& command,
    std::string& reasonCode)
{
    const auto session = sessionService_.find(command.legacyOsdSessionId);
    if (!session.has_value() ||
        session->actorId != command.actorId ||
        session->clientInstanceId != command.clientInstanceId ||
        session->backendId != command.backendId)
    {
        reasonCode = "legacy_osd_controller_session_invalid";
        return false;
    }
    if (session->sessionRevision != command.sessionRevision)
    {
        reasonCode = "revision_conflict";
        return false;
    }

    const auto viewer = viewerService_.find(command.viewerBindingId);
    if (!viewer.has_value() ||
        viewer->actorId != command.actorId ||
        viewer->clientInstanceId != command.clientInstanceId ||
        viewer->legacyOsdSessionId != command.legacyOsdSessionId ||
        viewer->backendId != command.backendId)
    {
        reasonCode = "legacy_osd_controller_viewer_invalid";
        return false;
    }

    OsdControllerStatusRequest status;
    status.actorId = command.actorId;
    status.clientInstanceId = command.clientInstanceId;
    status.backendId = command.backendId;
    status.legacyOsdSessionId = command.legacyOsdSessionId;
    status.viewerBindingId = command.viewerBindingId;
    const OsdControllerLeaseResult current = controllerService_.current(status);
    if (!current.accepted)
    {
        reasonCode = current.error.empty()
            ? "controller_lease_conflict" : current.error;
        return false;
    }
    if (!current.hasLease ||
        current.lease.state != OsdControllerLeaseState::Active ||
        current.lease.controllerLeaseId != command.controllerLeaseId ||
        current.lease.controllerLeaseEpoch != command.controllerLeaseEpoch ||
        current.lease.leaseRevision != command.leaseRevision)
    {
        reasonCode = "controller_lease_conflict";
        return false;
    }
    if (current.lease.backendGeneration != command.backendGeneration ||
        session->backendGeneration != command.backendGeneration ||
        viewer->backendGeneration != command.backendGeneration)
    {
        reasonCode = "generation_conflict";
        return false;
    }
    if (current.lease.osdSurfaceId != command.osdSurfaceId ||
        viewer->osdSurfaceId != command.osdSurfaceId)
    {
        reasonCode = "legacy_osd_controller_surface_changed";
        return false;
    }
    if (current.lease.osdEpoch != command.osdEpoch ||
        viewer->osdEpoch != command.osdEpoch ||
        session->osdEpoch != command.osdEpoch)
    {
        reasonCode = "legacy_osd_controller_epoch_changed";
        return false;
    }
    reasonCode.clear();
    return true;
}

bool LegacyOsdInputService::rateAllowed(
    const std::string& leaseId,
    std::int64_t now)
{
    std::lock_guard<std::mutex> lock(rateMutex_);
    RateWindow& window = rateWindows_[leaseId];
    if (window.second != now)
    {
        window.second = now;
        window.accepted = 0;
    }
    if (window.accepted >= MaximumAcceptedCommandsPerSecond)
        return false;
    ++window.accepted;
    return true;
}

LegacyOsdInputResult LegacyOsdInputService::submit(
    const LegacyOsdInputCommand& command)
{
    if (!legacyOsdInputCommandValid(command))
        return reject("legacy_osd_input_request_invalid");

    const std::int64_t now = nowProvider_();
    if (command.deadline <= now ||
        command.deadline - now > MaximumDeadlineLeadSeconds)
        return reject("legacy_osd_input_deadline_invalid");

    const std::string operationId = "osdi:" + command.inputCommandId;
    const auto existing = commandRepository_.findAssignmentForOperation(
        command.backendId, operationId, kLegacyOsdInputCommandType);
    if (existing.has_value())
    {
        LegacyOsdInputCommand existingCommand;
        if (!legacyOsdInputCommandParse(
                existing->payload, existingCommand) ||
            !sameSemanticCommand(existingCommand, command))
            return reject("legacy_osd_input_duplicate_conflict");

        LegacyOsdInputResult result;
        result.accepted = true;
        result.idempotent = true;
        result.category = "accepted_for_dispatch";
        result.agentCommandId = existing->commandId;
        return result;
    }

    std::string authorityReason;
    if (!authorityCurrent(command, authorityReason))
        return reject(authorityReason);

    const auto agent = agentRepository_.findAgentForBackend(command.backendId);
    if (!agent.has_value() || agent->revoked || agent->incompatible ||
        agent->agentInstanceId.empty() ||
        agent->backendGeneration != command.backendGeneration ||
        agent->leaseExpiresAt <= now)
        return reject("legacy_osd_input_agent_unavailable");

    if (!commandRepository_.hasCapability(
            command.backendId,
            agent->agentId,
            agent->agentInstanceId,
            agent->backendGeneration,
            kLegacyOsdInputCommandType))
        return reject("legacy_osd_input_capability_unavailable");

    if (!rateAllowed(command.controllerLeaseId, now))
        return reject("rate_limited");

    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = backendAgentGenerateOpaqueId("req_", 8);
    assignment.correlationId = command.correlationId;
    assignment.operationId = operationId;
    assignment.jobId = backendAgentGenerateOpaqueId("job_", 12);
    assignment.attemptId = backendAgentGenerateOpaqueId("att_", 12);
    assignment.claimEpoch = 1;
    assignment.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
    assignment.backendId = command.backendId;
    assignment.agentId = agent->agentId;
    assignment.agentInstanceId = agent->agentInstanceId;
    assignment.backendGeneration = agent->backendGeneration;
    assignment.commandType = kLegacyOsdInputCommandType;
    assignment.payloadVersion = kLegacyOsdInputPayloadVersion;
    assignment.payload = legacyOsdInputCommandSerialize(command);
    assignment.verificationPolicy = "dispatch_only";
    assignment.assignedAt = now;
    assignment.deadline = command.deadline;
    assignment.requestFingerprint =
        backendAgentCommandFingerprint(assignment);

    if (!backendAgentCommandValidAssignment(assignment) ||
        !commandRepository_.insertAssignment(assignment))
        return reject("legacy_osd_input_assignment_failed");

    LegacyOsdInputResult result;
    result.accepted = true;
    result.category = "accepted_for_dispatch";
    result.agentCommandId = assignment.commandId;
    return result;
}

bool LegacyOsdInputService::revalidateReceipt(
    const BackendAgentCommandReceipt& receipt,
    std::string& reasonCode)
{
    const auto assignment =
        commandRepository_.findAssignment(receipt.commandId);
    if (!assignment.has_value() ||
        assignment->commandType != kLegacyOsdInputCommandType ||
        assignment->requestFingerprint != receipt.requestFingerprint)
    {
        reasonCode = "legacy_osd_input_assignment_fenced";
        return false;
    }

    LegacyOsdInputCommand command;
    if (!legacyOsdInputCommandParse(assignment->payload, command))
    {
        reasonCode = "legacy_osd_input_payload_invalid";
        return false;
    }
    const std::int64_t now = nowProvider_();
    if (assignment->deadline <= now || command.deadline <= now)
    {
        reasonCode = "legacy_osd_input_deadline_expired";
        return false;
    }
    return authorityCurrent(command, reasonCode);
}
