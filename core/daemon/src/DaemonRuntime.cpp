#include "DaemonRuntime.h"

#include "ContinueWatchingApiRuntime.h"
#include "DaemonRuntimeRecordingEditing.h"
#include "DaemonHbbtvRuntime.h"
#include "DaemonLegacyOsdRuntime.h"
#include "DaemonTeletextRuntime.h"
#include "GenreBrowserApiRuntime.h"
#include "GlobalSearchApiRuntime.h"
#include "LiveRemoteApiRuntime.h"
#include "RecordingMediaHttpRuntime.h"
#include "PublicApiRuntime.h"
#include "MediaImportScheduleWindow.h"
#include "BackendAgentMediaImportAssignment.h"
#include "MediaImportOperationFence.h"
#include "MediaShrinkAgentCommand.h"
#include "MutationOperationRepository.h"
#include "Database.h"
#include <sqlite3.h>
#include "SeriesArtworkSettingsApiRuntime.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>

std::atomic<bool> DaemonRuntime::shutdownRequested_(false);

DaemonRuntime::DaemonRuntime()
    : initialized_(false),
      externalVdrChangeHint_(false),
      epgCacheWarmupStopRequested_(false),
      epgCacheDirtyHint_(false),
      recordingCacheWarmupStopRequested_(false)
{
}

int DaemonRuntime::run()
{
    if (!initialized_) {
        std::cerr << "vdr-suite-daemon runtime not initialized" << std::endl;
        return 1;
    }
    if (!httpServer_ || !apiRouter_ || !vdrRecordingQueryService_ || !vdrRecordingCacheRepository_) {
        std::cerr << "HTTP/API runtime unavailable for Media Gateway" << std::endl;
        return 1;
    }
    if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
        !embeddedBackendLifecycleService_ ||
        !backendAgentLifecycleService_ ||
        !configureDaemonTeletextRuntime(
            database_,
            *backendRegistryService_,
            *vdrSnapshotReadService_,
            *embeddedBackendLifecycleService_,
            *backendAgentLifecycleService_,
            backendRuntimeContexts_)) {
        std::cerr << "Teletext control-plane runtime unavailable" << std::endl;
        return 1;
    }
    if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
        !embeddedBackendLifecycleService_ ||
        !backendAgentLifecycleService_ ||
        !configureDaemonHbbtvRuntime(
            database_,
            config_.databasePath(),
            *backendRegistryService_,
            *vdrSnapshotReadService_,
            *embeddedBackendLifecycleService_,
            *backendAgentLifecycleService_,
            backendRuntimeContexts_)) {
        std::cerr << "HbbTV discovery control-plane runtime unavailable" << std::endl;
        return 1;
    }
    if (!backendAgentLifecycleService_ ||
        !backendAgentIdentityRepository_ ||
        !backendAgentRepository_ ||
        !backendAgentCommandRepository_ ||
        !backendAgentCommandDeliveryService_ ||
        !backendRegistryService_ ||
        !backendAccessPolicy_ ||
        !configureDaemonLegacyOsdRuntime(
            database_,
            *backendAgentIdentityRepository_,
            *backendAgentLifecycleService_,
            *backendAgentRepository_,
            *backendAgentCommandRepository_,
            *backendAgentCommandDeliveryService_,
            *backendRegistryService_,
            *backendAccessPolicy_)) {
        std::cerr
            << "Legacy OSD view-session runtime unavailable"
            << std::endl;
        return 1;
    }
    if (!ContinueWatchingApiRuntime::instance().configure(
            database_,
            *vdrRecordingCacheRepository_)) {
        std::cerr << "Continue Watching runtime unavailable" << std::endl;
        return 1;
    }
    if (!backendRegistryService_ || !backendAccessPolicy_ || !backendAgentRepository_ ||
        !backendAgentCommandRepository_ ||
        !configureDaemonRecordingEditingRuntime(*vdrRecordingCacheRepository_, backendRuntimeContexts_,
            *backendRegistryService_, *backendAccessPolicy_, *backendAgentRepository_,
            *backendAgentCommandRepository_)) {
        std::cerr << "Recording editing runtime unavailable" << std::endl; return 1;
    }
    // Recover finished, already durably recorded HEVC stages without a
    // browser GET and without blocking the HTTP/Agent control-plane thread.
    mediaShrinkRecoveryStopRequested_.store(false);
    mediaShrinkRecoveryThread_ = std::thread([this]() {
        runMediaShrinkRecoveryWorker();
    });

    auto lastVdrPoll = std::chrono::steady_clock::now();
    auto lastMediaImportPoll =
        std::chrono::steady_clock::now() - std::chrono::minutes(1);
    auto lastDurableBatchProgress =
        std::chrono::steady_clock::now() - std::chrono::seconds(3);
    const int runtimeExit = runRecordingMediaHttpRuntime(
        database_,
        *apiRouter_,
        *vdrRecordingQueryService_,
        httpServer_,
        httpListener_,
        config_.httpListenHost(),
        config_.httpListenPort(),
        []() {
            return shutdownRequested_.load();
        },
        [this, lastVdrPoll, lastMediaImportPoll,
         lastDurableBatchProgress]() mutable {
            publishCompletedRecordingRefreshes();
            publishRecordingPresentationChanges();
            // Continue Watching lives in actor-scoped SQLite state, not the
            // VDR snapshot. Emit only backend-scoped invalidation after a
            // confirmed Browser/Device commit; authorized clients read back
            // their own progress through the existing API.
            const auto progressBackends =
                ContinueWatchingApiRuntime::instance().takePendingProgressBackends();
            if (snapshotChangeFeed_ && snapshotChangeFeedService_ &&
                liveTransportService_ && snapshotCacheService_) {
                for (const auto& backendId : progressBackends) {
                    snapshotChangeFeedService_->appendChanges(
                        *snapshotChangeFeed_,
                        snapshotCacheService_->generation(),
                        {VdrChangeEvent(VdrChangeType::ContinueWatchingChanged)},
                        backendId);
                    liveTransportService_->publishChangeFeedEntry(
                        snapshotChangeFeed_->entries().back());
                }
            }
            const auto now = std::chrono::steady_clock::now();
            // Core owns continuation of already-confirmed, durably reserved
            // per-source imports. No browser/SSE connection drives dispatch.
            // This uses the existing daemon lifecycle tick, not a new worker.
            if (now - lastDurableBatchProgress >= std::chrono::seconds(2)) {
                lastDurableBatchProgress = now;
                try { progressDurableMediaImportBatches(); }
                catch (...) {
                    std::cerr << "Durable Media Import batch progression failed"
                              << std::endl;
                }
            }
            if (now - lastMediaImportPoll >= std::chrono::minutes(1)) {
                lastMediaImportPoll = now;
                try {
                    pollScheduledMediaImport();
                } catch (...) {
                    // A failed optional automation can never terminate the
                    // daemon's HTTP / recording service.
                    std::cerr << "Media Import automation failed closed"
                              << std::endl;
                }
            }
            const bool externalHint = externalVdrChangeHint_.exchange(false);

            if (!externalHint &&
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastVdrPoll).count() < 5) {
                return;
            }

            lastVdrPoll = now;
            pollVdrAndUpdateChangeFeed();
        });
    mediaShrinkRecoveryStopRequested_.store(true);
    if (mediaShrinkRecoveryThread_.joinable())
        mediaShrinkRecoveryThread_.join();
    return runtimeExit;
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
    while (!mediaShrinkRecoveryStopRequested_.load()) {
        try {
            if (mutationOperationRepository_ &&
                backendAgentCommandRepository_ &&
                mediaShrinkPlanRuntime_ && mediaShrinkReviewRepository_) {
                // Succeeded is included to finish registering a staged review
                // if the daemon crashed between the CAS and registry insert.
                const auto pending =
                    mutationOperationRepository_->listByActionFamilyAndStates(
                        "media.shrink.stage",
                        {State::outcomeUnknown, State::succeeded}, 32);
                if (pending.ok()) {
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
                        if (!assignment) continue;
                        const auto result =
                            backendAgentCommandRepository_->resultForCommand(
                                assignment->commandId);
                        if (!result) continue;
                        const auto now = duration_cast<seconds>(
                            system_clock::now().time_since_epoch()).count();
                        if (tryReconcileMediaShrinkStage(
                                *assignment, *result, now))
                            std::cout << "HEVC_STAGE_RECOVERY=VERIFIED"
                                      << " operation=" << op.operationId
                                      << std::endl;
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

    // All batch items live in the existing mutation_operations table.
    // Only the earliest not-yet-dispatched item whose EVERY predecessor has
    // the real 'succeeded' Core/VDR readback may be assigned. A failed or
    // unknown predecessor intentionally pauses the rest; never replay it.
    auto* db = const_cast<Database*>(
        mutationOperationRepository_->databaseIdentity());
    if (!db) return;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::vector<std::string> ready;
    {
        auto lease = db->acquireTransactionLease();
        const std::string pattern = std::string("mb1_") +
            std::string(32U, '@') + "_[0-9][0-9][0-9]";
        std::string glob;
        for (char ch : pattern)
            glob += ch == '@' ? "[0-9a-f]" : std::string(1U, ch);
        const char* sql =
            "SELECT m.operation_id FROM mutation_operations m "
            "WHERE m.action_family='media.import' AND m.state='accepted' "
            "AND m.idempotency_key GLOB ? AND m.deadline>? "
            "AND NOT EXISTS("
            "SELECT 1 FROM mutation_operations p "
            "WHERE p.actor_id=m.actor_id AND p.backend_id=m.backend_id "
            "AND p.action_family='media.import' "
            "AND substr(p.idempotency_key,1,37)=substr(m.idempotency_key,1,37) "
            "AND p.idempotency_key<m.idempotency_key "
            "AND p.state!='succeeded') "
            "ORDER BY m.requested_at,m.idempotency_key LIMIT 8;";
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(db->handle(), sql, -1, &statement, nullptr)
                != SQLITE_OK || sqlite3_bind_text(
                    statement, 1, glob.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_int64(statement, 2, seconds) != SQLITE_OK) {
            if (statement) sqlite3_finalize(statement);
            return;
        }
        while (sqlite3_step(statement) == SQLITE_ROW) {
            const auto* raw = sqlite3_column_text(statement, 0);
            if (raw) ready.emplace_back(
                reinterpret_cast<const char*>(raw));
        }
        sqlite3_finalize(statement);
    }
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
