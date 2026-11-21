#include "DaemonRuntime.h"

// Existing daemon Media Tools lifecycle workers; this is a translation-unit
// extraction, not a new scheduler, operation state, table, or mutation path.
#include "BackendAgentMediaImportAssignment.h"
#include "BackendAgentMediaShrinkAssignment.h"
#include "DaemonMediaShrinkAdmissionAuthority.h"
#include "MediaImportOperationFence.h"
#include "MediaImportScheduleWindow.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaShrinkOperationAdmission.h"
#include "MediaShrinkReviewRepository.h"
#include "MutationOperationRepository.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

void DaemonRuntime::progressDurableMediaShrinkAutoPublish()
{
    using namespace vdrsuite::addons;
    using State = vdrsuite::operations::MutationOperationState;
    if (!mutationOperationRepository_ || !mediaShrinkReviewRepository_ ||
        !mediaShrinkOperationFence_ || !mediaShrinkPlanRuntime_ ||
        !backendAgentRepository_ || !backendAgentCommandRepository_)
        return;

    // The review repository owns the SQLite join and bounded selection.
    const auto stageIds =
        mediaShrinkReviewRepository_->listAutoPublishStageOperationIds(128);

    if (stageIds.empty()) return;
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    DaemonMediaShrinkAdmissionAuthority authority(
        *mediaShrinkPlanRuntime_, *mediaShrinkOperationFence_,
        *mediaShrinkReviewRepository_);
    MediaShrinkOperationAdmissionService admissions(
        *mutationOperationRepository_, *mediaShrinkReviewRepository_,
        authority);
    RequestSecurityContext system;
    system.authenticationState = AuthenticationState::Authenticated;
    system.actor = {"media_shrink_auto_publish", ActorType::System,
                    "HEVC Stage automatic publication", true};
    vdrsuite::agent::BackendAgentMediaShrinkAssignmentService assigner(
        *mutationOperationRepository_, *backendAgentCommandRepository_,
        *backendAgentRepository_, *mediaShrinkOperationFence_);

    for (const auto& stageId : stageIds) {
        const auto stageResult =
            mutationOperationRepository_->findById(stageId);
        const auto payload =
            mutationOperationRepository_->findPayloadByOperationId(stageId);
        if (!stageResult.ok() || !payload.ok()) continue;
        const auto& stage = stageResult.operation;
        if (stage.state != State::succeeded ||
            stage.actionFamily != "media.shrink.stage" ||
            stage.resourceType != "Recording" ||
            payload.payload.payloadType !=
                vdrsuite::mediatools::kMediaShrinkStageCommandType ||
            payload.payload.payloadVersion != 1)
            continue;
        vdrsuite::mediatools::MediaShrinkAgentSelection selection;
        if (!vdrsuite::mediatools::parseMediaShrinkAgentSelection(
                payload.payload.payloadType, payload.payload.payload,
                selection) ||
            !selection.autoPublish ||
            selection.recordingId != stage.resourceId ||
            !vdrsuite::mediatools::shrinkOpaqueId(
                stage.resultReference, "shk_"))
            continue;
        const auto review = mediaShrinkReviewRepository_->find(
            stage.actorId, stage.backendId, stage.resultReference);
        if (!review.ok() ||
            review.review.stageOperationId != stage.operationId ||
            review.review.recordingId != stage.resourceId ||
            review.review.sourceFingerprint !=
                stage.expectedResourceFingerprint)
            continue;

        if (review.review.state == "staged" &&
            review.review.revision == 1) {
            MediaShrinkPublishAdmission request;
            request.actorId = stage.actorId;
            request.backendId = stage.backendId;
            request.reviewId = review.review.reviewId;
            request.expectedRevision = 1;
            request.idempotencyKey = stage.operationId;
            request.now = now;
            const auto result = admissions.publish(request);
            if (result.status == MediaShrinkAdmissionStatus::accepted &&
                result.operation.state == State::accepted) {
                const auto dispatched = assigner.assign(
                    system, result.operation.operationId, now);
                // Wake subscribers of the original Stage; the browser
                // discovers the existing Publish operation by readback.
                if (dispatched.accepted && deviceLiveEventHub_)
                    deviceLiveEventHub_->publishOperation(stage.operationId);
            }
        } else if (review.review.state == "publishing" &&
                   review.review.revision == 2) {
            // Restore the single accepted command after a Core restart.
            // Never replay a dispatched or outcome-unknown mutation.
            const auto existing = mutationOperationRepository_->findById(
                review.review.publishOperationId);
            if (!existing.ok() ||
                existing.operation.state != State::accepted ||
                existing.operation.actionFamily != "media.shrink.publish" ||
                existing.operation.actorId != stage.actorId ||
                existing.operation.backendId != stage.backendId ||
                existing.operation.resourceId != review.review.reviewId)
                continue;
            if (!backendAgentCommandRepository_->findAssignmentForOperation(
                    stage.backendId, existing.operation.operationId,
                    vdrsuite::mediatools::kMediaShrinkPublishCommandType)) {
                const auto dispatched = assigner.assign(
                    system, existing.operation.operationId, now);
                if (dispatched.accepted && deviceLiveEventHub_)
                    deviceLiveEventHub_->publishOperation(stage.operationId);
            }
        }
    }
}

void DaemonRuntime::runMediaShrinkRecoveryWorker()
{
    using State = vdrsuite::operations::MutationOperationState;
    using namespace std::chrono;
    // Backend-owned readback rather than browser polling. All attempts
    // operate on the ORIGINAL durable Agent assignment and exact candidate.
    // No new command or FFmpeg execution can be dispatched from this worker.
    const auto waitForStop = [this](int seconds) {
        for (int i = 0; i < seconds; ++i) {
            if (mediaShrinkRecoveryStopRequested_.load()) return true;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return mediaShrinkRecoveryStopRequested_.load();
    };
    if (waitForStop(2)) return;
    // Report a reason once per distinct operation/reason. A long-lived
    // uncertain Stage must not spam journald at every maintenance tick.
    std::map<std::string, std::string> reported;
    const auto report = [&reported](const std::string& operationId,
                                    const std::string& reason) {
        if (reason.empty() || reported[operationId] == reason) return;
        reported[operationId] = reason;
        std::cerr << "HEVC_STAGE_RECOVERY=BLOCKED"
                  << " operation=" << operationId
                  << " reason=" << reason << std::endl;
    };
    while (!mediaShrinkRecoveryStopRequested_.load()) {
        try {
            if (mutationOperationRepository_ &&
                backendAgentCommandRepository_ &&
                mediaShrinkPlanRuntime_ && mediaShrinkReviewRepository_) {
                // Prioritize uncertain operations: old successful reviews
                // must never starve recovery by filling the bounded scan.
                auto pending =
                    mutationOperationRepository_->listByActionFamilyAndStates(
                        "media.shrink.stage", {State::outcomeUnknown}, 32);
                // Also heal a crash between Stage CAS and review registration.
                const auto completed =
                    mutationOperationRepository_->listByActionFamilyAndStates(
                        "media.shrink.stage", {State::succeeded}, 16);
                if (pending.ok() && completed.ok()) {
                    pending.operations.insert(pending.operations.end(),
                        completed.operations.begin(),
                        completed.operations.end());
                    for (const auto& op : pending.operations) {
                        if (mediaShrinkRecoveryStopRequested_.load()) break;
                        // Completed reviews need no further media probing.
                        // Only a crashed CAS->register gap needs readback.
                        if (op.state == State::succeeded &&
                            mediaShrinkReviewRepository_->find(
                                op.actorId, op.backendId,
                                op.resultReference).ok())
                            continue;
                        const auto assignment =
                            backendAgentCommandRepository_->
                                findAssignmentForOperation(
                                    op.backendId, op.operationId,
                                    vdrsuite::mediatools::
                                        kMediaShrinkStageCommandType);
                        if (!assignment) {
                            report(op.operationId, "agent_assignment_missing");
                            continue;
                        }
                        const auto result =
                            backendAgentCommandRepository_->resultForCommand(
                                assignment->commandId);
                        if (!result) {
                            report(op.operationId, "durable_agent_result_missing");
                            continue;
                        }
                        const auto now = duration_cast<seconds>(
                            system_clock::now().time_since_epoch()).count();
                        std::string diagnostic;
                        if (tryReconcileMediaShrinkStage(
                                *assignment, *result, now, &diagnostic)) {
                            reported.erase(op.operationId);
                            std::cout << "HEVC_STAGE_RECOVERY=VERIFIED"
                                      << " operation=" << op.operationId
                                      << std::endl;
                        } else {
                            report(op.operationId, diagnostic);
                        }
                    }
                }
            }
        } catch (const std::exception&) {
            // Avoid leaking file paths; the authoritative operation stays
            // untouched for the next independent readback.
            std::cerr << "HEVC_STAGE_RECOVERY=RETRYABLE" << std::endl;
        } catch (...) {
            std::cerr << "HEVC_STAGE_RECOVERY=RETRYABLE" << std::endl;
        }
        if (waitForStop(60)) break;
    }
}

void DaemonRuntime::progressDurableMediaImportBatches()
{
    if (!mutationOperationRepository_ || !backendAgentCommandRepository_ ||
        !backendAgentRepository_ || !mediaImportOperationFence_)
        return;

    // The mutation operation repository owns the exact predecessor fence.
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const auto ready =
        mutationOperationRepository_->listReadyMediaImportBatchIds(seconds);

    if (ready.empty()) return;
    RequestSecurityContext system;
    system.authenticationState = AuthenticationState::Authenticated;
    system.actor = {"media_import_batch_dispatch", ActorType::System,
                    "Media Import batch progression", true};
    vdrsuite::agent::BackendAgentMediaImportAssignmentService assigner(
        *mutationOperationRepository_, *backendAgentCommandRepository_,
        *backendAgentRepository_, *mediaImportOperationFence_);
    // Progress each eligible batch at most one item. All authoritative
    // revisions, actor grants, backend and Agent generation are rechecked by
    // the SAME existing Agent assignment service as a normal user POST.
    for (const auto& operationId : ready)
        (void)assigner.assign(system, operationId, seconds);
}

void DaemonRuntime::pollScheduledMediaImport()
{
    // Administrator opt-in: no autonomous imports unless ALL THREE values
    // are configured, and the selected existing Service actor has an active
    // backend-scoped addons.media.import grant and enabled add-on intent.
    const char* actorEnv = std::getenv("VDR_SUITE_MEDIA_IMPORT_AUTO_ACTOR");
    const char* backendEnv = std::getenv("VDR_SUITE_MEDIA_IMPORT_AUTO_BACKEND");
    const char* windowEnv = std::getenv("VDR_SUITE_MEDIA_IMPORT_AUTO_WINDOW");
    if (!actorEnv || !backendEnv || !windowEnv ||
        !mutationOperationRepository_ || !backendAgentRepository_ ||
        !backendAgentIdentityRepository_ || !mediaImportOperationFence_ ||
        !mediaImportCatalog_)
        return;
    const std::string actor(actorEnv), backend(backendEnv), window(windowEnv);
    if (actor.empty() || actor.size() > 128 ||
        backend.empty() || backend.size() > 128) return;
    const auto configured =
        vdrsuite::mediatools::MediaImportScheduleWindow::parse(window);
    if (!configured) return;
    const auto now = std::chrono::system_clock::now();
    const auto unixSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    const std::time_t localSeconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    if (!::localtime_r(&localSeconds, &local)) return;
    const int clockMinutes = local.tm_hour * 60 + local.tm_min;
    if (!configured->contains(clockMinutes)) return;

    // A configured human-account ID is not a scheduler credential. Only a
    // provisioned Service identity with independently managed Core grants
    // can act as the durable operation owner.
    const auto identity = backendAgentIdentityRepository_->findActor(actor);
    if (!identity || identity->type != ActorType::Service ||
        !identity->active || identity->revoked) return;

    using State = vdrsuite::operations::MutationOperationState;
    const auto outstanding = mutationOperationRepository_
        ->listByActionFamilyAndStates("media.import",
            {State::accepted, State::queued, State::dispatching,
             State::executedUnverified, State::outcomeUnknown}, 256);
    if (!outstanding.ok() || outstanding.operations.size() >= 256) return;
    for (const auto& operation : outstanding.operations)
        if (operation.backendId == backend) return;

    std::string reason;
    if (!mediaImportOperationFence_->effectiveMediaImportCapability(
            actor, backend, unixSeconds, reason)) return;
    const auto agent = backendAgentRepository_->findAgentForBackend(backend);
    if (!agent) return;
    std::vector<std::string> sourceIds;
    if (!mediaImportCatalog_->list(*agent, backend, unixSeconds, sourceIds))
        return;
    for (const auto& sourceId : sourceIds) {
        const std::string key = "auto_" + sourceId;
        const auto prior = mutationOperationRepository_->findByIdempotencyScope(
            actor, backend, "MediaImportSource", sourceId, "media.import", key);
        if (prior.status == vdrsuite::operations::MutationOperationRepositoryStatus::ok) {
            if (prior.operation.state == State::succeeded) continue;
            // Ambiguous and failed results require human review, not retries
            // or a second unattended operation with a different key.
            return;
        }
        if (prior.status !=
            vdrsuite::operations::MutationOperationRepositoryStatus::notFound)
            return;
        PublicMediaImportAdmissionRequest request;
        request.actorRef = actor;
        request.backendId = backend;
        request.sourceId = sourceId;
        request.idempotencyKey = key;
        // Uses the exact same Core admission+Agent assignment as API v1.
        // One reservation at most per poll; the next poll waits for canonical
        // reconciliation of this operation before selecting another source.
        (void)admitMediaImport(request);
        return;
    }
}
