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
#include "SeriesArtworkSettingsApiRuntime.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
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
    auto lastVdrPoll = std::chrono::steady_clock::now();
    auto lastMediaImportPoll =
        std::chrono::steady_clock::now() - std::chrono::minutes(1);
    return runRecordingMediaHttpRuntime(
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
        [this, lastVdrPoll, lastMediaImportPoll]() mutable {
            publishCompletedRecordingRefreshes();
            publishRecordingPresentationChanges();
            const auto now = std::chrono::steady_clock::now();
            if (now - lastMediaImportPoll >= std::chrono::minutes(1)) {
                lastMediaImportPoll = now;
                pollScheduledMediaImport();
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
        backend.empty() || backend.size() > 128 ||
        window.size() != 11 || window[2] != ':' || window[5] != '-' ||
        window[8] != ':') return;
    for (const int index : {0,1,3,4,6,7,9,10})
        if (window[index] < '0' || window[index] > '9') return;
    const auto minutes = [&](std::size_t offset) {
        const int hour = (window[offset] - '0') * 10 +
                         (window[offset + 1] - '0');
        const int minute = (window[offset + 3] - '0') * 10 +
                           (window[offset + 4] - '0');
        if (hour > 23 || minute > 59) return -1;
        return hour * 60 + minute;
    };
    const int begin = minutes(0), finish = minutes(6);
    if (begin < 0 || finish < 0 || begin == finish) return;
    const auto now = std::chrono::system_clock::now();
    const auto unixSeconds = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    const std::time_t localSeconds = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    if (!::localtime_r(&localSeconds, &local)) return;
    const int clockMinutes = local.tm_hour * 60 + local.tm_min;
    const bool inWindow = begin < finish
        ? clockMinutes >= begin && clockMinutes < finish
        : clockMinutes >= begin || clockMinutes < finish;
    if (!inWindow) return;

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
