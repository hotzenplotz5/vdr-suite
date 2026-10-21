#include "MediaImportOperationFence.h"

#include "AuthorizationService.h"
#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistryService.h"
#include "MediaToolsAgentExecutor.h"
#include "MutationOperationRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <utility>

namespace vdrsuite::addons {
namespace {
bool exactReceipt(const BackendAgentCommandAssignment& a,
                  const BackendAgentCommandReceipt& r)
{
    return r.commandId == a.commandId &&
        r.requestFingerprint == a.requestFingerprint &&
        r.jobId == a.jobId && r.attemptId == a.attemptId &&
        r.claimEpoch == a.claimEpoch && r.backendId == a.backendId &&
        r.agentId == a.agentId && r.agentInstanceId == a.agentInstanceId &&
        r.backendGeneration == a.backendGeneration &&
        r.receiptCategory == "accepted";
}

bool denied(std::string& reason, const char* code)
{
    reason = code;
    return false;
}
} // namespace

MediaImportOperationFence::MediaImportOperationFence(
    vdrsuite::operations::MutationOperationRepository& operations,
    SecurityIdentityRepository& identities,
    SecurityPermissionGrantRepository& grants,
    BackendAgentRepository& agents,
    BackendAgentCommandRepository& commands,
    BackendRegistryService& backendRegistry,
    BackendAccessPolicy& backendAccess)
    : operations_(operations), identities_(identities), grants_(grants),
      agents_(agents), commands_(commands),
      backendRegistry_(backendRegistry), backendAccess_(backendAccess)
{
}

bool MediaImportOperationFence::check(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandReceipt& receipt,
    std::int64_t now, std::string& reason) const
{
    reason.clear();
    std::string sourceId;
    if (now <= 0 || !backendAgentCommandValidAssignment(assignment) ||
        !backendAgentCommandValidReceipt(receipt) ||
        assignment.commandType != vdrsuite::mediatools::kMediaImportCommandType ||
        assignment.payloadVersion != 1 ||
        assignment.verificationPolicy != "reconciliation_required" ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            assignment.payload, sourceId) ||
        !exactReceipt(assignment, receipt) ||
        receipt.receivedAt > now || assignment.deadline <= now ||
        assignment.assignedAt > now)
        return denied(reason, "media_import_invalid_receipt_fence");

    // A live Agent identity, instance, generation and explicitly published
    // capability are necessary, but never sufficient for authorization.
    const auto agent = agents_.findAgentForBackend(assignment.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->agentId != assignment.agentId ||
        agent->agentInstanceId != assignment.agentInstanceId ||
        agent->backendGeneration != assignment.backendGeneration ||
        agent->leaseExpiresAt < now ||
        !commands_.hasCapability(
            assignment.backendId, assignment.agentId,
            assignment.agentInstanceId, assignment.backendGeneration,
            assignment.commandType))
        return denied(reason, "media_import_agent_fenced");

    const auto operation = operations_.findById(assignment.operationId);
    if (!operation.ok())
        return denied(reason, "media_import_operation_missing");
    const auto& op = operation.operation;
    // Operation requestFingerprint is the *logical API request* fingerprint;
    // the Agent command fingerprint is independently computed and validated.
    if (op.state !=
            vdrsuite::operations::MutationOperationState::dispatching ||
        op.actionFamily != "media.import" ||
        op.resourceType != "MediaImportSource" ||
        op.resourceId != sourceId ||
        op.verificationPolicy !=
            vdrsuite::operations::MutationOperationVerificationPolicy::
                reconciliationRequired ||
        op.backendId != assignment.backendId ||
        op.backendGeneration != assignment.backendGeneration ||
        op.requestedAt > assignment.assignedAt ||
        op.deadline == 0 || op.deadline < assignment.deadline ||
        op.deadline <= now || !op.resultReference.empty() ||
        op.actorId.empty() || op.requestFingerprint.empty())
        return denied(reason, "media_import_operation_fenced");

    // The original source token is read from the immutable ADR-0042 payload,
    // NOT from a caller-selected pathname, and must exactly match the Agent
    // assignment. The operation repository owns idempotency/revisions.
    const auto payload = operations_.findPayloadByOperationId(op.operationId);
    if (!payload.ok() || payload.payload.payloadType != "addon.media.import" ||
        payload.payload.payloadVersion != 1 ||
        payload.payload.payload != assignment.payload ||
        payload.payload.payloadFingerprint.empty())
        return denied(reason, "media_import_payload_conflict");

    const auto actor = identities_.findActor(op.actorId);
    if (!actor || !actor->active || actor->revoked ||
        (actor->type != ActorType::User &&
         actor->type != ActorType::Service))
        return denied(reason, "media_import_actor_revoked");

    const auto grants = grants_.findActiveGrantsForActor(op.actorId);
    if (!grants.available)
        return denied(reason, "media_import_grants_unavailable");

    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = {actor->actorId, actor->type, actor->displayName, true};
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    context.grants = grants.grants;
    const AuthorizationService auth;
    if (!auth.authorize(context, {
            "addons.media.import", assignment.backendId, "media.import"
        }).allowed)
        return denied(reason, "media_import_permission_denied");

    const auto backend = backendAccess_.canWriteToBackend(
        backendRegistry_, assignment.backendId);
    if (!backend.backendFound || !backend.allowed || backend.readOnly)
        return denied(reason, "media_import_backend_unavailable");

    reason = "media_import_operation_and_grant_verified";
    return true;
}

} // namespace vdrsuite::addons
