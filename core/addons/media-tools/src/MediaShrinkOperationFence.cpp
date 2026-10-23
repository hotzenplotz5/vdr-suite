#include "MediaShrinkOperationFence.h"
#include "AddonMediaImportMaintenanceRead.h"

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
#include <utility>

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
    MediaShrinkReviewRepository& reviews,
    std::function<std::string(const std::string&, std::int64_t)> trustedProviderCheck)
    : operations_(operations), identities_(identities), grants_(grants),
      agents_(agents), commands_(commands),
      backendRegistry_(backendRegistry), backendAccess_(backendAccess),
      activationIntents_(activationIntents), reviews_(reviews),
      trustedProviderCheck_(std::move(trustedProviderCheck)) {}

bool MediaShrinkOperationFence::applyResult(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandResult& result,
    std::int64_t now, std::string& reason) const
{
    using State = vdrsuite::operations::MutationOperationState;
    using Policy = vdrsuite::operations::MutationOperationVerificationPolicy;
    reason.clear();
    const bool stage = assignment.commandType ==
        vdrsuite::mediatools::kMediaShrinkStageCommandType;
    const bool publish = assignment.commandType ==
        vdrsuite::mediatools::kMediaShrinkPublishCommandType;
    if (now <= 0 || (!stage && !publish) ||
        !backendAgentCommandValidAssignment(assignment) ||
        !backendAgentCommandValidResult(result) ||
        result.commandId != assignment.commandId ||
        result.jobId != assignment.jobId ||
        result.attemptId != assignment.attemptId ||
        result.claimEpoch != assignment.claimEpoch ||
        result.backendId != assignment.backendId ||
        result.agentId != assignment.agentId ||
        result.agentInstanceId != assignment.agentInstanceId ||
        result.backendGeneration != assignment.backendGeneration ||
        result.requestFingerprint != assignment.requestFingerprint ||
        result.completedAt < assignment.assignedAt ||
        result.completedAt > now)
        return deny(reason, "media_shrink_result_fence_invalid");

    // The command journal has already committed this exact result. Replays
    // may only repeat the same immutable journal entry.
    const auto recorded = commands_.resultForCommand(assignment.commandId);
    if (!recorded ||
        backendAgentCommandResultIdentity(*recorded) !=
            backendAgentCommandResultIdentity(result))
        return deny(reason, "media_shrink_result_not_durable");
    const auto selected = operations_.findById(assignment.operationId);
    if (!selected.ok())
        return deny(reason, "media_shrink_result_operation_missing");
    const auto& op = selected.operation;
    const auto payload = operations_.findPayloadByOperationId(op.operationId);
    vdrsuite::mediatools::MediaShrinkAgentSelection selection;
    if (!payload.ok() ||
        payload.payload.payloadVersion != 1 ||
        payload.payload.payloadType != assignment.commandType ||
        payload.payload.payload != assignment.payload ||
        !vdrsuite::mediatools::parseMediaShrinkAgentSelection(
            assignment.commandType, assignment.payload, selection) ||
        op.backendId != assignment.backendId ||
        op.backendGeneration != assignment.backendGeneration ||
        op.operationId != assignment.operationId ||
        op.verificationPolicy !=
            (stage ? Policy::readbackRequired : Policy::reconciliationRequired) ||
        op.actionFamily !=
            (stage ? "media.shrink.stage" : "media.shrink.publish") ||
        op.resourceType != (stage ? "Recording" : "MediaShrinkReview") ||
        assignment.verificationPolicy !=
            (stage ? "readback_required" : "reconciliation_required") ||
        op.resourceId != (stage ? selection.recordingId : selection.reviewId) ||
        op.expectedResourceFingerprint.empty())
        return deny(reason, "media_shrink_result_operation_conflict");

    const bool hasEvidence = !result.resultEvidence.empty();
    if (hasEvidence &&
        (!vdrsuite::mediatools::shrinkOpaqueId(
             result.resultEvidence, "shk_") ||
         (publish && result.resultEvidence != selection.reviewId)))
        return deny(reason, "media_shrink_result_evidence_invalid");

    State target = State::outcomeUnknown;
    if (result.resultCategory == "outcome_unknown" &&
        result.verificationState == "outcome_unknown" &&
        result.retryClassification == "reconcile_only" &&
        ((result.dispatchState == "accepted_by_executor" && hasEvidence) ||
         (result.dispatchState == "starting" && !hasEvidence)))
        target = State::outcomeUnknown;
    else if (result.resultCategory == "rejected" &&
             result.verificationState == "outcome_unknown" &&
             result.retryClassification == "none" &&
             !hasEvidence &&
             result.dispatchState == "not_started" &&
             (result.errorCategory == "expired" ||
              result.errorCategory == "fenced"))
        // The canonical assignment is already dispatching, whose legal
        // terminal rejection is failedVerified (not failedBeforeDispatch).
        target = State::failedVerified;
    else if (result.resultCategory == "rejected" &&
             result.verificationState == "outcome_unknown" &&
             result.retryClassification == "none" &&
             !hasEvidence &&
             result.dispatchState == "starting" &&
             result.errorCategory == "fenced")
        target = State::failedVerified;
    else
        return deny(reason, "media_shrink_result_requires_reconciliation");

    // No result is allowed to promote the canonical operation to succeeded:
    // Stage requires a separately validated private review; Publish requires
    // independent VDR Recording reconciliation and a new rec_ identity.
    const std::string reference =
        "agent.media.shrink:" + assignment.commandId;
    if (op.state == target && op.resultReference == reference) {
        reason = "media_shrink_result_already_applied";
        return true;
    }
    if (op.state == State::succeeded &&
        target == State::outcomeUnknown &&
        result.dispatchState == "accepted_by_executor" &&
        (stage ? op.resultReference == result.resultEvidence :
                 vdrsuite::mediatools::shrinkOpaqueId(
                     op.resultReference, "rec_"))) {
        // An ACK lost after independent reconciliation cannot roll a
        // verified operation backward or strand the Agent on result replay.
        reason = "media_shrink_result_already_reconciled";
        return true;
    }
    if (op.state != State::dispatching)
        return deny(reason, "media_shrink_result_state_conflict");
    const auto applied = operations_.transition(
        op.operationId, op.operationRevision, State::dispatching,
        target, reference, now);
    if (applied.ok()) {
        reason = "media_shrink_result_applied_unverified";
        return true;
    }
    return deny(reason, "media_shrink_result_transition_failed");
}

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
    // Effective capability disappears under the global maintenance freeze.
    // Unavailable SQLite/schema must deny, not be interpreted as unfrozen.
    auto* maintenanceDatabase =
        const_cast<Database*>(operations_.databaseIdentity());
    if (!maintenanceDatabase ||
        !mediaToolsAdmissionUnfrozen(*maintenanceDatabase))
        return deny(reason, "media_shrink_maintenance_frozen");

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
    // Effective activation is tied to an explicitly approved Agent lease,
    // not just a persisted desired-enabled bit and self-reported capability.
    if (activation.intent.backendGeneration != agent->backendGeneration ||
        activation.intent.agentId != agent->agentId ||
        activation.intent.agentInstanceId != agent->agentInstanceId ||
        activation.intent.backendGeneration == 0)
        return deny(reason, "media_shrink_activation_agent_stale");

    if (!trustedProviderCheck_)
        return deny(reason, "media_shrink_provider_untrusted");
    const std::string trustedSha = trustedProviderCheck_(backendId, now);
    if (trustedSha.empty() ||
        trustedSha != activation.intent.approvedHandlerSha256)
        return deny(reason, "media_shrink_provider_untrusted");
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
    // The command receipt is the final mutation gate: a reserved or queued
    // HEVC command may not start after an administrator freezes Media Tools.
    auto* maintenanceDatabase =
        const_cast<Database*>(operations_.databaseIdentity());
    if (!maintenanceDatabase ||
        !mediaToolsAdmissionUnfrozen(*maintenanceDatabase))
        return deny(reason, "media_shrink_maintenance_frozen");

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
    // Receipt must honor the same exact administrator-approved Agent lease
    // already required by effectiveCapability (not only desiredEnabled).
    if (activation.intent.backendGeneration == 0 ||
        activation.intent.backendGeneration != agent->backendGeneration ||
        activation.intent.agentId != agent->agentId ||
        activation.intent.agentInstanceId != agent->agentInstanceId)
        return deny(reason, "media_shrink_activation_agent_stale");

    // A successful command reservation is not a durable execution grant.
    // Re-evaluate the exact Core-approved backend provider at receipt.
    if (!trustedProviderCheck_)
        return deny(reason, "media_shrink_provider_untrusted");
    const std::string trustedSha =
        trustedProviderCheck_(assignment.backendId, now);
    if (trustedSha.empty() ||
        trustedSha != activation.intent.approvedHandlerSha256)
        return deny(reason, "media_shrink_provider_untrusted");

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
