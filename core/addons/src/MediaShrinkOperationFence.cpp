#include "MediaShrinkOperationFence.h"

#include "AddonActivationIntentRepository.h"
#include "AddonPermissionCatalog.h"
#include "AuthorizationService.h"
#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistryService.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaShrinkCommandFence.h"
#include "MediaShrinkReviewRepository.h"
#include "MutationOperationRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

namespace vdrsuite::addons {
namespace {
bool deny(std::string& reason, const char* code) {
    reason = code;
    return false;
}
bool exactReceipt(const BackendAgentCommandAssignment& a,
                  const BackendAgentCommandReceipt& r) {
    return backendAgentCommandValidReceipt(r) &&
        r.commandId == a.commandId &&
        r.requestFingerprint == a.requestFingerprint &&
        r.jobId == a.jobId && r.attemptId == a.attemptId &&
        r.claimEpoch == a.claimEpoch && r.backendId == a.backendId &&
        r.agentId == a.agentId && r.agentInstanceId == a.agentInstanceId &&
        r.backendGeneration == a.backendGeneration &&
        r.receiptCategory == "accepted";
}
} // namespace

MediaShrinkOperationFence::MediaShrinkOperationFence(
    vdrsuite::operations::MutationOperationRepository& operations,
    SecurityIdentityRepository& identities,
    SecurityPermissionGrantRepository& grants,
    BackendAgentRepository& agents,
    BackendAgentCommandRepository& commands,
    BackendRegistryService& backendRegistry,
    BackendAccessPolicy& backendAccess,
    const AddonActivationIntentRepository& activationIntents,
    MediaShrinkReviewRepository& reviews)
    : operations_(operations), identities_(identities), grants_(grants),
      agents_(agents), commands_(commands),
      backendRegistry_(backendRegistry), backendAccess_(backendAccess),
      activationIntents_(activationIntents), reviews_(reviews) {}

bool MediaShrinkOperationFence::effectiveCapability(
    const std::string& actorId, const std::string& backendId,
    const std::string& commandType, std::int64_t now,
    std::uint64_t& generation, std::string& reason) const
{
    generation = 0;
    const bool stage = commandType ==
        vdrsuite::mediatools::kMediaShrinkStageCommandType;
    const bool publish = commandType ==
        vdrsuite::mediatools::kMediaShrinkPublishCommandType;
    if (now <= 0 || actorId.empty() || backendId.empty() ||
        backendId == "*" || (!stage && !publish))
        return deny(reason, "media_shrink_invalid_admission");

    // The exact Agent executable capability and its current lease are
    // mandatory. An unregistered or disabled add-on cannot authorize work.
    const auto agent = agents_.findAgentForBackend(backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->agentId.empty() || agent->agentInstanceId.empty() ||
        agent->backendGeneration == 0 || agent->leaseExpiresAt < now ||
        !commands_.hasCapability(
            backendId, agent->agentId, agent->agentInstanceId,
            agent->backendGeneration, commandType))
        return deny(reason, "media_shrink_agent_unavailable");

    if (!reviewedAddonPermission("addons.media.shrink"))
        return deny(reason, "media_shrink_permission_not_enabled");
    const auto actor = identities_.findActor(actorId);
    if (!actor || !actor->active || actor->revoked ||
        (actor->type != ActorType::User && actor->type != ActorType::Service))
        return deny(reason, "media_shrink_actor_unavailable");
    const auto grants = grants_.findActiveGrantsForActor(actorId);
    if (!grants.available)
        return deny(reason, "media_shrink_grants_unavailable");
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = {actor->actorId, actor->type, actor->displayName, true};
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    context.grants = grants.grants;
    const AuthorizationService authorization;
    if (!authorization.authorize(context, {
            "addons.media.shrink", backendId,
            stage ? "media.shrink.stage" : "media.shrink.publish"
        }).allowed)
        return deny(reason, "media_shrink_separate_grant_denied");
    const auto backend = backendAccess_.canWriteToBackend(
        backendRegistry_, backendId);
    if (!backend.backendFound || !backend.allowed || backend.readOnly)
        return deny(reason, "media_shrink_backend_not_writable");
    const auto activation = activationIntents_.readForAdmission(
        "rectools", backendId);
    if (activation.status != IntentStatus::ok ||
        !activation.intent.desiredEnabled || activation.intent.revision <= 0)
        return deny(reason, "media_shrink_activation_disabled");

    generation = agent->backendGeneration;
    reason = "media_shrink_core_admission_authorized";
    return true;
}

bool MediaShrinkOperationFence::check(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandReceipt& receipt,
    std::int64_t now, std::string& reason) const
{
    if (now <= 0 || !backendAgentCommandValidAssignment(assignment) ||
        !exactReceipt(assignment, receipt) || receipt.receivedAt > now ||
        assignment.deadline <= now)
        return deny(reason, "media_shrink_invalid_receipt");

    // Reserved command and current local Agent advertisement MUST agree with
    // the one live lease/generation. No stale Agent result can activate work.
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
        return deny(reason, "media_shrink_agent_fenced");

    const auto operation = operations_.findById(assignment.operationId);
    if (!operation.ok())
        return deny(reason, "media_shrink_operation_missing");
    const auto payload = operations_.findPayloadByOperationId(
        assignment.operationId);
    if (!payload.ok() ||
        inspectMediaShrinkCommandFence(
            operation.operation, payload.payload, assignment, now) !=
                MediaShrinkCommandFenceResult::candidateOnly)
        return deny(reason, "media_shrink_core_operation_fenced");

    const auto actor = identities_.findActor(operation.operation.actorId);
    if (!actor || !actor->active || actor->revoked ||
        (actor->type != ActorType::User &&
         actor->type != ActorType::Service))
        return deny(reason, "media_shrink_actor_unavailable");
    const auto grants = grants_.findActiveGrantsForActor(actor->actorId);
    if (!grants.available)
        return deny(reason, "media_shrink_grants_unavailable");
    // Until Core explicitly reviews enabling this additional mutation right,
    // even an old manually persisted grant cannot make the addon executable.
    if (!reviewedAddonPermission("addons.media.shrink"))
        return deny(reason, "media_shrink_permission_not_enabled");

    RequestSecurityContext request;
    request.authenticationState = AuthenticationState::Authenticated;
    request.actor = {actor->actorId, actor->type, actor->displayName, true};
    request.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    request.grants = grants.grants;
    const AuthorizationService authorization;
    if (!authorization.authorize(request, {
            "addons.media.shrink", assignment.backendId,
            assignment.commandType == vdrsuite::mediatools::kMediaShrinkStageCommandType
                ? "media.shrink.stage" : "media.shrink.publish"
        }).allowed)
        return deny(reason, "media_shrink_separate_grant_denied");

    const auto backend = backendAccess_.canWriteToBackend(
        backendRegistry_, assignment.backendId);
    if (!backend.backendFound || !backend.allowed || backend.readOnly)
        return deny(reason, "media_shrink_backend_not_writable");

    const auto activation = activationIntents_.readForAdmission(
        "rectools", assignment.backendId);
    if (activation.status != IntentStatus::ok ||
        !activation.intent.desiredEnabled || activation.intent.revision <= 0)
        return deny(reason, "media_shrink_activation_disabled");

    if (assignment.commandType ==
        vdrsuite::mediatools::kMediaShrinkPublishCommandType)
    {
        const auto stored = reviews_.find(actor->actorId, assignment.backendId,
                                         operation.operation.resourceId);
        if (!stored.ok() || stored.review.state != "publishing" ||
            stored.review.publishOperationId != operation.operation.operationId ||
            stored.review.backendGeneration != assignment.backendGeneration ||
            stored.review.sourceFingerprint !=
                operation.operation.expectedResourceFingerprint ||
            stored.review.revision != 2 ||
            operation.operation.expectedRevision != "1")
            return deny(reason, "media_shrink_candidate_not_claimed");
    }
    reason = "media_shrink_core_operation_grant_and_lease_verified";
    return true;
}

} // namespace vdrsuite::addons
