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
            *vdrRecordingCacheRepository_,
            [this](const std::string& backendId, const std::string& nativeId)
            {
                if (!publicRecordingIdentities_) return std::string{};
                return publicRecordingIdentities_->resolveOrCreate(backendId, nativeId)
                    .value_or(std::string{});
            })) {
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
    int runtimeExit = 1;
    try {
    runtimeExit = runRecordingMediaHttpRuntime(
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
            // The Agent already publishes its bounded source inventory.
            // Emit one backend-scoped SSE invalidation only when the actual
            // catalog contents changed, never on every Agent heartbeat.
            // Clients read their normal authenticated source list afterward.
            if (mediaImportCatalog_ && snapshotChangeFeed_ &&
                snapshotChangeFeedService_ && liveTransportService_ &&
                snapshotCacheService_) {
                for (const auto& backendId :
                     mediaImportCatalog_->takeChangedSourceBackends()) {
                    snapshotChangeFeedService_->appendChanges(
                        *snapshotChangeFeed_,
                        snapshotCacheService_->generation(),
                        {VdrChangeEvent(VdrChangeType::MediaImportSourcesChanged)},
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
                try { progressDurableMediaShrinkAutoPublish(); }
                catch (...) {
                    std::cerr << "Durable HEVC auto-publication progression failed"
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
            publishPendingHbbtvPresentationChanges();
            const bool externalHint = externalVdrChangeHint_.exchange(false);

            if (!externalHint &&
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastVdrPoll).count() < 5) {
                return;
            }

            lastVdrPoll = now;
            pollVdrAndUpdateChangeFeed();
        });
    } catch (...) {
        mediaShrinkRecoveryStopRequested_.store(true);
        if (mediaShrinkRecoveryThread_.joinable())
            mediaShrinkRecoveryThread_.join();
        throw;
    }
    mediaShrinkRecoveryStopRequested_.store(true);
    if (mediaShrinkRecoveryThread_.joinable())
        mediaShrinkRecoveryThread_.join();
    return runtimeExit;
}

