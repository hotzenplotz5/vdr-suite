#pragma once

#include "ApiRouter.h"
#include "BackendRuntimeContext.h"
#include "BackendPollingCoordinator.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "BackendRegistryJsonSerializer.h"
#include "BackendRegistryController.h"
#include "DashboardController.h"
#include "DashboardFacade.h"
#include "DashboardJsonSerializer.h"
#include "ConsoleRuntimeLogger.h"
#include "Database.h"
#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "IHttpServer.h"
#include "JobDashboardService.h"
#include "JobRepository.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "PollingService.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"
#include "RecordingsController.h"
#include "RuntimeConfig.h"
#include "RuntimeDiagnosticsController.h"
#include "RuntimeDiagnosticsJsonSerializer.h"
#include "RuntimeDiagnosticsService.h"
#include "SnapshotAccessService.h"
#include "SnapshotChangeFeed.h"
#include "SnapshotChangeFeedController.h"
#include "SnapshotChangeFeedJsonSerializer.h"
#include "SnapshotChangeFeedService.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrSnapshotReadService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrConfig.h"
#include "VdrController.h"
#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"
#include "SimpleHttpListener.h"

#include <atomic>
#include <memory>
#include <vector>

class DaemonRuntime
{
public:
    DaemonRuntime();

    bool initialize();
    int run();
    void shutdown();

private:
    static void handleSignal(int signalNumber);
    void pollVdrAndUpdateChangeFeed();
    std::unique_ptr<BackendRuntimeContext> createBackendRuntimeContext(
        const BackendNode& backend);

    bool initialized_;

    RuntimeConfig config_;
    ConsoleRuntimeLogger runtimeLogger_;
    RuntimeDiagnosticsService runtimeDiagnosticsService_;
    Database database_;

    std::unique_ptr<JobRepository> jobRepository_;
    std::unique_ptr<RecordingRepository> recordingRepository_;
    std::unique_ptr<MetadataRepository> metadataRepository_;

    std::unique_ptr<JobDashboardService> jobDashboardService_;
    std::unique_ptr<RecordingDashboardService> recordingDashboardService_;
    std::unique_ptr<DashboardFacade> dashboardFacade_;

    std::unique_ptr<DashboardJsonSerializer> dashboardJsonSerializer_;
    std::unique_ptr<DashboardController> dashboardController_;
    std::unique_ptr<JobsController> jobsController_;
    std::unique_ptr<RecordingsController> recordingsController_;
    std::unique_ptr<MetadataController> metadataController_;

    BackendRegistry backendRegistry_;
    std::unique_ptr<BackendRegistryService> backendRegistryService_;
    std::unique_ptr<BackendRegistryJsonSerializer> backendRegistryJsonSerializer_;
    std::unique_ptr<BackendRegistryController> backendRegistryController_;
    VdrConfig vdrConfig_;
    std::vector<std::unique_ptr<BackendRuntimeContext>> backendRuntimeContexts_;
    std::unique_ptr<BackendPollingCoordinator> backendPollingCoordinator_;
    std::unique_ptr<SnapshotCache> snapshotCache_;
    std::unique_ptr<SnapshotCacheService> snapshotCacheService_;
    std::unique_ptr<SnapshotAccessService> snapshotAccessService_;
    std::unique_ptr<VdrSnapshotReadService> vdrSnapshotReadService_;
    std::unique_ptr<VdrSnapshotReadJsonSerializer> vdrSnapshotReadJsonSerializer_;
    std::unique_ptr<SnapshotChangeFeed> snapshotChangeFeed_;
    std::unique_ptr<SnapshotChangeFeedService> snapshotChangeFeedService_;
    std::unique_ptr<SnapshotChangeFeedJsonSerializer> snapshotChangeFeedJsonSerializer_;
    std::unique_ptr<SnapshotChangeFeedController> snapshotChangeFeedController_;
    std::unique_ptr<VdrOverviewService> vdrOverviewService_;
    std::unique_ptr<VdrOverviewJsonSerializer> vdrOverviewJsonSerializer_;
    std::unique_ptr<VdrController> vdrController_;

    std::unique_ptr<RuntimeDiagnosticsJsonSerializer> runtimeDiagnosticsJsonSerializer_;
    std::unique_ptr<RuntimeDiagnosticsController> runtimeDiagnosticsController_;

    std::unique_ptr<ApiRouter> apiRouter_;
    std::unique_ptr<IHttpServer> httpServer_;
    std::unique_ptr<SimpleHttpListener> httpListener_;

    static std::atomic<bool> shutdownRequested_;
};
