#include "MediaImportOperationFence.h"

#include "AddonActivationIntentRepository.h"
#include "AuthorizationService.h"
#include "BackendAccessPolicy.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistryService.h"
#include "MediaToolsAgentExecutor.h"
#include "AddonMediaImportMaintenanceRead.h"
#include "MediaImportRecordingEvidence.h"
#include "MutationOperationRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <utility>
#include <filesystem>
#include <system_error>
#include <limits>
#include <ctime>
#include <iomanip>
#include <sstream>

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
    BackendAccessPolicy& backendAccess,
    const AddonActivationIntentRepository& activationIntents,
    std::function<std::string(const std::string&, std::int64_t)> trustedProviderCheck)
    : operations_(operations), identities_(identities), grants_(grants),
      agents_(agents), commands_(commands),
      backendRegistry_(backendRegistry), backendAccess_(backendAccess),
      activationIntents_(activationIntents),
      trustedProviderCheck_(std::move(trustedProviderCheck))
{
}

bool MediaImportOperationFence::check(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandReceipt& receipt,
    std::int64_t now, std::string& reason) const
{
    if (!backendAgentCommandValidReceipt(receipt) ||
        !exactReceipt(assignment, receipt) ||
        receipt.receivedAt > now)
        return denied(reason, "media_import_invalid_receipt_fence");
    return checkCore(assignment, now, true, reason);
}

bool MediaImportOperationFence::checkCandidate(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now, std::string& reason) const
{
    return checkCore(assignment, now, false, reason);
}

bool MediaImportOperationFence::actorBackendAllowed(
    const std::string& actorId, const std::string& backendId,
    std::int64_t now, std::string& reason) const
{
    // Shared by authenticated API discovery and immutable operation dispatch.
    const auto actor = identities_.findActor(actorId);
    if (!actor || !actor->active || actor->revoked ||
        (actor->type != ActorType::User &&
         actor->type != ActorType::Service))
        return denied(reason, "media_import_actor_revoked");

    const auto grants = grants_.findActiveGrantsForActor(actorId);
    if (!grants.available)
        return denied(reason, "media_import_grants_unavailable");

    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = {actor->actorId, actor->type, actor->displayName, true};
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    context.grants = grants.grants;
    const AuthorizationService auth;
    if (!auth.authorize(context, {
            "addons.media.import", backendId, "media.import"
        }).allowed)
        return denied(reason, "media_import_permission_denied");

    const auto backend = backendAccess_.canWriteToBackend(
        backendRegistry_, backendId);
    if (!backend.backendFound || !backend.allowed || backend.readOnly)
        return denied(reason, "media_import_backend_unavailable");

    auto* database = const_cast<Database*>(operations_.databaseIdentity());
    if (database == nullptr ||
        !mediaImportAdmissionUnfrozen(*database))
        return denied(reason, "media_import_maintenance_frozen");

    // A previously issued per-actor grant must never revive a disabled
    // add-on. Reuse the one audited per-backend desired-state repository.
    // This is an additional necessary gate, NEVER sufficient permission.
    const auto intent = activationIntents_.readForAdmission(
        "rectools", backendId);
    if (intent.status != IntentStatus::ok ||
        (intent.intent.revision > 0 && !intent.intent.desiredEnabled))
        return denied(reason, "media_import_activation_disabled");

    // Installation and root-handler approval do not imply activation.
    // A revision-bound administrator enable and the current handler/Agent
    // binding remain mandatory on every admission and dispatch path.
    const auto activeAgent = agents_.findAgentForBackend(backendId);
    if (!activeAgent || activeAgent->revoked || activeAgent->incompatible ||
        !trustedProviderCheck_)
        return denied(reason, "media_import_provider_untrusted");
    const std::string trustedSha = trustedProviderCheck_(backendId, now);
    if (trustedSha.empty() ||
        (intent.intent.revision > 0 && intent.intent.desiredEnabled &&
         trustedSha != intent.intent.approvedHandlerSha256))
        return denied(reason, "media_import_provider_untrusted");
    if (!mediaToolsActivationPermits(
            intent.intent, activeAgent->agentId,
            activeAgent->agentInstanceId, activeAgent->backendGeneration,
            trustedSha))
        return denied(reason, "media_import_activation_agent_stale");

    reason = "media_import_actor_backend_authorized";
    return true;
}

bool MediaImportOperationFence::effectiveMediaImportCapability(
    const std::string& actorId, const std::string& backendId,
    std::int64_t now, std::string& reason) const
{
    reason.clear();
    if (now <= 0 || actorId.empty() || backendId.empty() ||
        backendId == "*" || actorId == "*")
        return denied(reason, "media_import_capability_invalid_context");
    const auto agent = agents_.findAgentForBackend(backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->agentId.empty() || agent->agentInstanceId.empty() ||
        agent->backendGeneration == 0 || agent->leaseExpiresAt < now ||
        !commands_.hasCapability(
            backendId, agent->agentId, agent->agentInstanceId,
            agent->backendGeneration,
            vdrsuite::mediatools::kMediaImportCommandType))
        return denied(reason, "media_import_capability_agent_unavailable");
    if (!actorBackendAllowed(actorId, backendId, now, reason)) return false;
    reason = "media_import_effective_capability_available";
    return true;
}

bool MediaImportOperationFence::checkCore(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now, bool requireDispatching, std::string& reason) const
{
    reason.clear();
    std::string sourceId;
    vdrsuite::mediatools::MediaImportDecision choice;
    if (now <= 0 || !backendAgentCommandValidAssignment(assignment) ||
        assignment.commandType != vdrsuite::mediatools::kMediaImportCommandType ||
        assignment.verificationPolicy != "reconciliation_required" ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            assignment.payload, sourceId, choice) ||
        assignment.payloadVersion !=
            vdrsuite::mediatools::mediaImportSelectionVersion(choice, assignment.payload) ||
        assignment.deadline <= now || assignment.assignedAt > now)
        return denied(reason, "media_import_invalid_assignment_fence");

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
    using OperationState = vdrsuite::operations::MutationOperationState;
    if ((requireDispatching
            ? op.state != OperationState::dispatching
            : (op.state != OperationState::accepted &&
               op.state != OperationState::dispatching)) ||
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
        payload.payload.payloadVersion != assignment.payloadVersion ||
        payload.payload.payload != assignment.payload ||
        payload.payload.payloadFingerprint.empty())
        return denied(reason, "media_import_payload_conflict");

    if (!actorBackendAllowed(op.actorId, assignment.backendId, now, reason))
        return false;

    reason = "media_import_operation_and_grant_verified";
    return true;
}


bool MediaImportOperationFence::applyResult(
    const BackendAgentCommandAssignment& assignment,
    const BackendAgentCommandResult& result,
    std::int64_t now, std::string& reason) const
{
    reason.clear();
    std::string recordingProof, sourceProof;
    const bool recordedPath = vdrsuite::mediatools::parseMediaImportResultEvidence(
        result.resultEvidence, recordingProof, sourceProof);
    if (now <= 0 || !backendAgentCommandValidAssignment(assignment) ||
        !backendAgentCommandValidResult(result) ||
        assignment.commandType != vdrsuite::mediatools::kMediaImportCommandType ||
        assignment.verificationPolicy != "reconciliation_required" ||
        result.commandId != assignment.commandId ||
        result.requestFingerprint != assignment.requestFingerprint ||
        result.jobId != assignment.jobId ||
        result.attemptId != assignment.attemptId ||
        result.claimEpoch != assignment.claimEpoch ||
        result.backendId != assignment.backendId ||
        result.agentId != assignment.agentId ||
        result.agentInstanceId != assignment.agentInstanceId ||
        result.backendGeneration != assignment.backendGeneration ||
        result.completedAt < assignment.assignedAt ||
        result.completedAt > now ||
        (!result.resultEvidence.empty() &&
         !recordedPath &&
         result.resultEvidence.compare(0, 17, "media-tools-mark:") != 0))
        return denied(reason, "media_import_result_fence_invalid");

    // The existing command/result journal, not the callback argument, is
    // the authoritative persisted Agent outcome.
    const auto durable = commands_.resultForCommand(result.commandId);
    if (!durable || backendAgentCommandResultIdentity(*durable) !=
                        backendAgentCommandResultIdentity(result))
        return denied(reason, "media_import_result_not_durable");

    const auto operation = operations_.findById(assignment.operationId);
    if (!operation.ok())
        return denied(reason, "media_import_result_operation_missing");
    const auto& op = operation.operation;
    std::string sourceId;
    vdrsuite::mediatools::MediaImportDecision choice;
    const auto payload = operations_.findPayloadByOperationId(op.operationId);
    if (op.backendId != assignment.backendId ||
        op.backendGeneration != assignment.backendGeneration ||
        op.actionFamily != "media.import" ||
        op.resourceType != "MediaImportSource" ||
        op.verificationPolicy !=
            vdrsuite::operations::MutationOperationVerificationPolicy::
                reconciliationRequired ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            assignment.payload, sourceId, choice) ||
        assignment.payloadVersion !=
            vdrsuite::mediatools::mediaImportSelectionVersion(choice, assignment.payload) ||
        op.resourceId != sourceId || !payload.ok() ||
        payload.payload.payloadType != "addon.media.import" ||
        payload.payload.payloadVersion != assignment.payloadVersion ||
        payload.payload.payload != assignment.payload)
        return denied(reason, "media_import_result_operation_conflict");

    using State = vdrsuite::operations::MutationOperationState;
    State target = State::outcomeUnknown;
    // The authenticated backend Agent verifies a source rename with fsync
    // and readback. This is a source-marker outcome, NOT a VDR recording.
    const bool markerVerified =
        (choice == vdrsuite::mediatools::MediaImportDecision::desktop ||
         choice == vdrsuite::mediatools::MediaImportDecision::skip ||
         choice == vdrsuite::mediatools::MediaImportDecision::duplicate ||
         choice == vdrsuite::mediatools::MediaImportDecision::deleteDuplicate ||
         choice == vdrsuite::mediatools::MediaImportDecision::release) &&
        result.dispatchState == "accepted_by_executor" &&
        result.verificationState == "verified" &&
        result.resultCategory == "succeeded" &&
        result.errorCategory == "none" &&
        result.retryClassification == "none" &&
        result.resultEvidence ==
            vdrsuite::mediatools::mediaImportMarkerEvidence(sourceId, choice);
    if (markerVerified)
        target = State::executedUnverified;
    else if (result.resultCategory == "outcome_unknown" &&
        result.verificationState == "outcome_unknown" &&
        result.retryClassification == "reconcile_only" &&
        (result.dispatchState == "starting" ||
         result.dispatchState == "accepted_by_executor"))
        target = State::outcomeUnknown;
    else if (result.resultCategory == "rejected" &&
             result.verificationState == "outcome_unknown" &&
             result.retryClassification == "none" &&
             result.dispatchState == "not_started" &&
             (result.errorCategory == "expired" ||
              result.errorCategory == "fenced"))
        target = State::failedBeforeDispatch;
    else if (result.resultCategory == "rejected" &&
             result.verificationState == "outcome_unknown" &&
             result.retryClassification == "none" &&
             result.dispatchState == "starting" &&
             result.errorCategory == "fenced")
        target = State::failedVerified;
    else
        return denied(reason, "media_import_result_requires_reconciliation");

    const std::string reference = "agent.media.import:" + assignment.commandId;
    if (((!markerVerified && op.state == target) ||
         (markerVerified && op.state == State::succeeded)) &&
        op.resultReference == reference)
    {
        reason = "media_import_result_already_applied";
        return true;
    }
    // Agent result ACK may be lost after the native cache independently
    // finished reconciliation. Exact replay must not roll succeeded back or
    // strand the Agent in an endless 409 acknowledgement loop.
    if (op.state == State::succeeded &&
        op.resultReference == reference &&
        target == State::outcomeUnknown &&
        recordedPath)
    {
        reason = "media_import_result_already_verified";
        return true;
    }
    if (markerVerified && op.state == State::executedUnverified &&
        op.resultReference == reference) {
        const auto finished = operations_.transition(
            op.operationId, op.operationRevision, State::executedUnverified,
            State::succeeded, reference, now);
        if (finished.ok()) {
            reason = "media_import_marker_verified";
            return true;
        }
        return denied(reason, "media_import_marker_transition_failed");
    }
    if (op.state != State::dispatching)
        return denied(reason, "media_import_result_state_conflict");

    const auto applied = operations_.transition(
        op.operationId, op.operationRevision, State::dispatching,
        target, reference, now);
    if (applied.status ==
            vdrsuite::operations::MutationOperationRepositoryStatus::ok ||
        applied.status ==
            vdrsuite::operations::MutationOperationRepositoryStatus::
                idempotentReplay)
    {
        if (markerVerified) {
            // Preserve the canonical dispatching -> executed_unverified ->
            // succeeded lifecycle. Replay completes an interrupted second CAS.
            const auto current = operations_.findById(op.operationId);
            if (!current.ok())
                return denied(reason, "media_import_marker_missing");
            const auto finished = operations_.transition(
                op.operationId, current.operation.operationRevision,
                State::executedUnverified, State::succeeded, reference, now);
            if (!finished.ok())
                return denied(reason, "media_import_marker_transition_failed");
        }
        reason = markerVerified
            ? "media_import_marker_verified" : "media_import_result_applied";
        return true;
    }
    return denied(reason, "media_import_result_transition_failed");
}


namespace {
std::int64_t sqliteUtcSeconds(const std::string& timestamp)
{
    if (timestamp.size() != 19) return 0;
    std::tm parsed{};
    std::istringstream in(timestamp);
    in >> std::get_time(&parsed, "%Y-%m-%d %H:%M:%S");
    if (in.fail() || in.peek() != std::char_traits<char>::eof())
        return 0;
    const auto unixTime = ::timegm(&parsed);
    return unixTime > 0 ? static_cast<std::int64_t>(unixTime) : 0;
}

// Provider recording.path is normally a recording-relative display path
// (e.g. /Serien/Andor/...rec), NOT the absolute .rec path emitted by the
// importer. backendNativeId is the provider's full recording identity;
// it can contain a VDR video-root symlink such as /srv/vdr/video ->
// /srv/vdr/video.00. The importing Agent hashes the literal installed
// destination (/srv/vdr/video.00/...rec), so compare all *observed* forms.
// Never infer a root from a title, construct a path from browser data, or
// treat an unmatched recording as confirmation.
bool recordingIdentityMatches(
    const VdrRecording& recording, const std::string& evidence)
{
    using vdrsuite::mediatools::recordingPathEvidence;
    if (recordingPathEvidence(recording.path) == evidence ||
        recordingPathEvidence(recording.backendNativeId) == evidence)
        return true;

    // Canonicalize only a complete native recording identity that exists
    // locally. For a remote backend, canonical() fails harmlessly and only
    // the literal provider identities can match; no filesystem mutation.
    const std::filesystem::path native(recording.backendNativeId);
    if (!native.is_absolute() || native.extension() != ".rec" ||
        native.lexically_normal() != native)
        return false;
    std::error_code error;
    const auto canonical = std::filesystem::canonical(native, error);
    return !error && recordingPathEvidence(canonical.string()) == evidence;
}

bool exactRecordingIdentity(
    const std::vector<VdrRecording>& recordings,
    const std::string& backendId, const std::string& evidence)
{
    std::size_t matches = 0;
    for (const auto& recording : recordings)
    {
        if (recording.backendId != backendId ||
            recording.id.empty() || recording.backendNativeId.empty() ||
            recording.title.empty())
            continue;
        if (recordingIdentityMatches(recording, evidence))
            ++matches;
        if (matches > 1) return false;
    }
    return matches == 1;
}
}

std::size_t MediaImportOperationFence::reconcileRecordingCache(
    const std::string& backendId,
    const VdrRecordingCacheStatus& cacheStatus,
    const std::vector<VdrRecording>& recordings,
    std::int64_t now) const
{
    using State = vdrsuite::operations::MutationOperationState;
    if (backendId.empty() || backendId == "*" || cacheStatus.backendId != backendId ||
        cacheStatus.state != "ready" || !cacheStatus.lastError.empty() ||
        now <= 0)
        return 0;
    const auto refreshStarted = sqliteUtcSeconds(cacheStatus.startedAt);
    const auto refreshFinished = sqliteUtcSeconds(cacheStatus.finishedAt);
    if (refreshStarted <= 0 || refreshFinished < refreshStarted ||
        refreshFinished > now)
        return 0;

    // This is a bounded read of the existing operation repository after the
    // provider cache transaction has completed, never a worker queue.
    const auto pending = operations_.listByActionFamilyAndStates(
        "media.import", {State::outcomeUnknown, State::executedUnverified}, 256);
    if (!pending.ok()) return 0;

    std::size_t verified = 0;
    static const std::string prefix = "agent.media.import:";
    for (const auto& op : pending.operations)
    {
        if (op.backendId != backendId ||
            op.resultReference.compare(0, prefix.size(), prefix) != 0)
            continue;
        const auto commandId = op.resultReference.substr(prefix.size());
        if (!backendAgentCommandSafeIdentifier(commandId)) continue;
        const auto assignment = commands_.findAssignment(commandId);
        const auto result = commands_.resultForCommand(commandId);
        std::string recordingProof, sourceProof;
        if (!assignment || !result ||
            assignment->commandId != result->commandId ||
            assignment->operationId != op.operationId ||
            assignment->backendId != backendId ||
            assignment->backendGeneration != op.backendGeneration ||
            assignment->commandType !=
                vdrsuite::mediatools::kMediaImportCommandType ||
            result->requestFingerprint != assignment->requestFingerprint ||
            result->backendGeneration != assignment->backendGeneration ||
            result->claimEpoch != assignment->claimEpoch ||
            result->resultCategory != "outcome_unknown" ||
            result->dispatchState != "accepted_by_executor" ||
            result->verificationState != "outcome_unknown" ||
            result->retryClassification != "reconcile_only" ||
            !vdrsuite::mediatools::parseMediaImportResultEvidence(
                result->resultEvidence, recordingProof, sourceProof) ||
            // A cache snapshot that started before completion cannot prove
            // this import, even if it contains an older matching directory.
            refreshStarted <= result->completedAt)
            continue;

        const auto payload = operations_.findPayloadByOperationId(op.operationId);
        std::string sourceId;
        vdrsuite::mediatools::MediaImportDecision choice;
        if (!payload.ok() ||
            payload.payload.payloadType != "addon.media.import" ||
            payload.payload.payload != assignment->payload ||
            !vdrsuite::mediatools::parseMediaImportSelection(
                assignment->payload, sourceId, choice) ||
            payload.payload.payloadVersion !=
                vdrsuite::mediatools::mediaImportSelectionVersion(choice, assignment->payload) ||
            assignment->payloadVersion != payload.payload.payloadVersion ||
            op.resourceId != sourceId ||
            op.resourceType != "MediaImportSource" ||
            op.verificationPolicy !=
                vdrsuite::operations::MutationOperationVerificationPolicy::
                    reconciliationRequired ||
            !exactRecordingIdentity(
                recordings, backendId, recordingProof))
            continue;

        // Reserve the notification BEFORE transitioning to succeeded, so
        // a crash cannot lose the Core->Agent signal. Poll is independently
        // fenced to 'succeeded', hence a failed CAS never releases it.
        // Only a source-bound notice permits Agent-local cleanup.
        if (choice != vdrsuite::mediatools::MediaImportDecision::import &&
            choice != vdrsuite::mediatools::MediaImportDecision::local)
            continue;
        if (now > std::numeric_limits<std::int64_t>::max() - 600 ||
            !commands_.ensureMediaImportAssignmentSchema())
            continue;
        const auto notificationPayload = vdrsuite::mediatools::mediaImportReadbackPayload(
            sourceId, commandId, recordingProof, sourceProof);
        const auto existing = commands_.findAssignmentForOperation(
            backendId, op.operationId,
            vdrsuite::mediatools::kMediaImportReadbackCommandType);
        if (existing)
        {
            if (existing->payload != notificationPayload ||
                existing->agentId != assignment->agentId ||
                existing->agentInstanceId != assignment->agentInstanceId ||
                existing->backendGeneration != assignment->backendGeneration)
                continue;
        }
        else
        {
            BackendAgentCommandAssignment notice;
            notice.present = true;
            notice.requestId = backendAgentGenerateOpaqueId("req_", 8);
            notice.correlationId = notice.requestId;
            notice.operationId = op.operationId;
            notice.jobId = backendAgentGenerateOpaqueId("job_", 12);
            notice.attemptId = backendAgentGenerateOpaqueId("att_", 12);
            notice.claimEpoch = 1;
            notice.commandId = backendAgentGenerateOpaqueId("cmd_", 12);
            notice.backendId = backendId;
            notice.agentId = assignment->agentId;
            notice.agentInstanceId = assignment->agentInstanceId;
            notice.backendGeneration = assignment->backendGeneration;
            notice.commandType =
                vdrsuite::mediatools::kMediaImportReadbackCommandType;
            notice.payloadVersion = sourceProof.empty() ? 1 : 2;
            notice.payload = notificationPayload;
            notice.verificationPolicy = "none";
            notice.assignedAt = now;
            notice.deadline = now + 600;
            notice.requestFingerprint = backendAgentCommandFingerprint(notice);
            if (!backendAgentCommandValidAssignment(notice) ||
                (!commands_.insertAssignment(notice) &&
                 !commands_.findAssignmentForOperation(
                     backendId, op.operationId,
                     vdrsuite::mediatools::kMediaImportReadbackCommandType)))
                continue;
        }
        // Do not reinterpret successful ffmpeg execution as a verified
        // recording. This CAS happens ONLY after committed provider readback.
        const auto committed = operations_.transition(
            op.operationId, op.operationRevision, op.state,
            State::succeeded, op.resultReference, now);
        if (committed.ok()) ++verified;
    }
    return verified;
}

} // namespace vdrsuite::addons
