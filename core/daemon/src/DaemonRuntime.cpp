#include "DaemonRuntime.h"

#include "DaemonSqliteShutdownCancellation.h"
#include "GenreBrowserApiRuntime.h"
#include "GlobalSearchApiRuntime.h"
#include "LiveRemoteApiRuntime.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaGatewayHttpServer.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionRepository.h"
#include "SeriesArtworkSettingsApiRuntime.h"
#include "SimpleHttpListener.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <utility>

namespace
{

constexpr const char* MediaSessionWorkspaceRoot =
    "/var/cache/vdr-suite/media-sessions";

}

std::atomic<bool> DaemonRuntime::shutdownRequested_(false);

DaemonRuntime::DaemonRuntime()
    : initialized_(false),
      externalVdrChangeHint_(false),
      epgCacheWarmupStopRequested_(false),
      epgCacheDirtyHint_(false),
      recordingCacheWarmupStopRequested_(false),
      recordingCacheDirtyHint_(false),
      recordingCacheActionRefreshAttempts_(0)
{
}

int DaemonRuntime::run()
{
    if (!initialized_) {
        std::cerr << "vdr-suite-daemon runtime not initialized" << std::endl;
        return 1;
    }

    MediaSessionRepository mediaSessionRepository(database_);
    if (!mediaSessionRepository.ensureSchema()) {
        std::cerr << "failed to initialize MediaSession schema" << std::endl;
        return 1;
    }
    if (!mediaSessionRepository.recoverNonTerminalBundles()) {
        std::cerr << "failed to recover MediaSession runtime ownership" << std::endl;
        return 1;
    }

    if (!httpServer_) {
        std::cerr << "HTTP server runtime unavailable for Media Gateway" << std::endl;
        return 1;
    }

    MediaRouteLeaseRepository mediaRouteLeaseRepository(database_);
    MediaAccessGrantAuthenticator mediaAccessGrantAuthenticator(
        mediaSessionRepository);
    MediaHlsArtifactReader mediaHlsArtifactReader(
        MediaSessionWorkspaceRoot);

    // The listener created during initialize() has not started yet. Rebuild it
    // around the same already-composed HTTP server after inserting the Media
    // Gateway as the outermost media-specific fence. The gateway dependencies
    // below remain alive for the complete listener run and the gateway is
    // destroyed before these stack-owned dependencies leave scope.
    httpListener_.reset();
    httpServer_ = std::make_unique<MediaGatewayHttpServer>(
        std::move(httpServer_),
        mediaAccessGrantAuthenticator,
        mediaRouteLeaseRepository,
        mediaHlsArtifactReader);

    auto lastVdrPoll = std::chrono::steady_clock::now();
    httpListener_ = std::make_unique<SimpleHttpListener>(
        config_.httpListenHost(),
        config_.httpListenPort(),
        *httpServer_,
        []() {
            return shutdownRequested_.load();
        },
        [this, lastVdrPoll]() mutable {
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

    std::cout << "MediaSession persistence and restart recovery initialized" << std::endl;
    std::cout << "Media Gateway runtime initialized" << std::endl;
    std::cout << "vdr-suite-daemon runtime running" << std::endl;
    std::cout << "vdr-suite-daemon serving HTTP on " << config_.httpListenHost() << ":" << config_.httpListenPort() << std::endl;

    const int result = httpListener_->runUntilStopped();

    // Avoid leaving a gateway that references stack-owned media runtime
    // dependencies between run() and shutdown().
    httpListener_.reset();
    httpServer_.reset();
    return result;
}

void DaemonRuntime::shutdown()
{
    if (!initialized_) {
        return;
    }

    recordingCacheWarmupStopRequested_.store(true);
    epgCacheWarmupStopRequested_.store(true);
    {
        DaemonSqliteShutdownCancellation sqliteCancellation(
            database_.handle());
        stopRecordingCacheWarmupWorker();
        stopEpgCacheWarmupWorker();
    }

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (!backendRuntimeContext) {
            continue;
        }

        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->stop();
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            backendRuntimeContext->suiteBridgeAgentRuntime->stop();
        }
    }

    httpListener_.reset();
    httpServer_.reset();
    apiRouter_.reset();
    SeriesArtworkSettingsApiRuntime::instance().reset();
    GlobalSearchApiRuntime::instance().reset();
    GenreBrowserApiRuntime::instance().reset();
    LiveRemoteApiRuntime::instance().reset();

    std::cout << "HTTP server runtime stopped" << std::endl;

    liveTransportController_.reset();
    liveTransportService_.reset();
    liveTransport_.reset();
    snapshotChangeFeedController_.reset();
    runtimeDiagnosticsController_.reset();
    runtimeDiagnosticsJsonSerializer_.reset();
    epgSearchNativeFuzzyOperatorRefreshController_.reset();
    epgSearchNativeFuzzyOperatorRefreshService_.reset();
    epgSearchNativeFuzzyStaleProbeAdministrationController_.reset();
    epgSearchNativeFuzzyStaleProbeAdministrationService_.reset();
    epgSearchNativeFuzzyStartupRestoreService_.reset();
    epgSearchNativeFuzzyCapabilityFreshnessPolicy_.reset();
    epgSearchNativeFuzzyCapabilityDetector_.reset();
    epgSearchNativeFuzzyCapabilityRepository_.reset();
    epgCacheController_.reset();
    searchTimerPreviewEpgCacheRefreshController_.reset();
    searchTimerAutomationPreviewController_.reset();
    searchTimerAutomationDryRunResultJsonSerializer_.reset();
    searchTimerAutomationReadOnlyService_.reset();
    searchTimerRuntimeMutationPolicyExecutor_.reset();
    searchTimerCommandExecutor_.reset();
    searchTimerDeleteRequestParser_.reset();
    searchTimerDeleteResultJsonSerializer_.reset();
    searchTimerDeleteService_.reset();
    searchTimerUpdateRequestParser_.reset();
    searchTimerUpdateResultJsonSerializer_.reset();
    searchTimerUpdateService_.reset();
    searchTimerCreateRequestParser_.reset();
    searchTimerCreateResultJsonSerializer_.reset();
    searchTimerCreateService_.reset();
    searchTimerDiscoveryController_.reset();
    searchTimerDiscoveryJsonSerializer_.reset();
    searchTimerDiscoveryService_.reset();
    searchTimerDiscoveryProvider_.reset();
    searchTimerController_.reset();
    searchTimerResultJsonSerializer_.reset();
    searchTimerService_.reset();
    epgController_.reset();
    epgSearchResultJsonSerializer_.reset();
    epgSearchService_.reset();
    epgQueryService_.reset();
    vdrTimerActionController_.reset();
    recordingPersonSearchController_.reset();
    recordingPersonSearchResultJsonSerializer_.reset();
    recordingPersonSearchService_.reset();
    personController_.reset();
    personQueryResultJsonSerializer_.reset();
    personSearchService_.reset();
    personResolutionJsonSerializer_.reset();
    vdrTimerActionExecutorAdapterRegistry_.reset();
    vdrTimerActionRequestParser_.reset();
    vdrTimerActionResultJsonSerializer_.reset();
    vdrTimerActionExecutionService_.reset();
    vdrTimerActionService_.reset();
    recordingActionExecutionController_.reset();
    recordingActionBackendExecutorAdapterRegistry_.reset();
    recordingActionExecutionResultJsonSerializer_.reset();
    recordingActionExecutionService_.reset();
    recordingActionValidationController_.reset();
    recordingActionValidationRequestParser_.reset();
    recordingActionValidationResultJsonSerializer_.reset();
    recordingActionValidationService_.reset();
    capabilityController_.reset();
    capabilityReportJsonSerializer_.reset();
    capabilityReportService_.reset();
    capabilityReportBuilder_.reset();
    capabilityResolver_.reset();
    capabilitySet_.reset();
    backendRegistryController_.reset();
    backendRegistryJsonSerializer_.reset();
    backendAgentLifecycleService_.reset();
    backendAgentRepository_.reset();
    backendAgentAccountabilityRepository_.reset();
    backendAgentCredentialVerifierRepository_.reset();
    backendAgentProvisioningRepository_.reset();
    backendAgentIdentityRepository_.reset();
    backendRegistryService_.reset();
    vdrRecordingQueryController_.reset();
    vdrRecordingFolderController_.reset();
    vdrRecordingQueryResultJsonSerializer_.reset();
    vdrRecordingQueryService_.reset();
    vdrRecordingCacheRepository_.reset();
    vdrController_.reset();
    vdrOverviewJsonSerializer_.reset();
    vdrOverviewService_.reset();
    snapshotChangeFeedJsonSerializer_.reset();
    snapshotChangeFeedService_.reset();
    snapshotChangeFeed_.reset();
    backendPollingCoordinator_.reset();
    backendRuntimeContexts_.clear();
    epgCacheServiceRegistry_.reset();
    epgEventRepository_.reset();
    searchTimerPreviewEpgCacheRefreshServiceRegistry_.reset();
    searchTimerPreviewEpgCache_.reset();
    vdrSnapshotReadJsonSerializer_.reset();
    vdrSnapshotReadService_.reset();
    snapshotAccessService_.reset();
    snapshotCacheService_.reset();
    snapshotCache_.reset();
    metadataController_.reset();
    recordingsController_.reset();
    jobsController_.reset();
    dashboardController_.reset();
    dashboardJsonSerializer_.reset();
    dashboardFacade_.reset();
    recordingDashboardService_.reset();
    jobDashboardService_.reset();
    metadataRepository_.reset();
    recordingRepository_.reset();
    jobRepository_.reset();

    std::cout << "API router runtime stopped" << std::endl;
    std::cout << "REST controller runtime stopped" << std::endl;
    std::cout << "dashboard runtime stopped" << std::endl;

    database_.close();

    std::cout << "database closed" << std::endl;
    std::cout << "vdr-suite-daemon runtime shutting down" << std::endl;

    initialized_ = false;
}

void DaemonRuntime::handleSignal(int signalNumber)
{
    if (signalNumber == SIGINT || signalNumber == SIGTERM) {
        shutdownRequested_ = true;
    }
}
