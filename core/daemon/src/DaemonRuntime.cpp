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
#include "SeriesArtworkSettingsApiRuntime.h"

#include <chrono>
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
        !configureDaemonTeletextRuntime(
            *backendRegistryService_,
            *vdrSnapshotReadService_,
            *embeddedBackendLifecycleService_,
            backendRuntimeContexts_)) {
        std::cerr << "Teletext control-plane runtime unavailable" << std::endl;
        return 1;
    }
    if (!backendRegistryService_ || !vdrSnapshotReadService_ ||
        !embeddedBackendLifecycleService_ ||
        !configureDaemonHbbtvRuntime(
            config_.databasePath(),
            *backendRegistryService_,
            *vdrSnapshotReadService_,
            *embeddedBackendLifecycleService_,
            backendRuntimeContexts_)) {
        std::cerr << "HbbTV discovery control-plane runtime unavailable" << std::endl;
        return 1;
    }
    if (!backendAgentLifecycleService_ ||
        !backendAgentIdentityRepository_ ||
        !configureDaemonLegacyOsdRuntime(
            database_,
            *backendAgentIdentityRepository_,
            *backendAgentLifecycleService_)) {
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
        [this, lastVdrPoll]() mutable {
            publishCompletedRecordingRefreshes();
            const auto now = std::chrono::steady_clock::now();
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
