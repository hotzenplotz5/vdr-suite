#include "DaemonRuntime.h"

#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "SimpleHttpListener.h"
#include "TestHttpServer.h"

#include <csignal>
#include <iostream>

std::atomic<bool> DaemonRuntime::shutdownRequested_(false);

DaemonRuntime::DaemonRuntime()
    : initialized_(false)
{
}

bool DaemonRuntime::initialize()
{
    std::cout << "vdr-suite-daemon runtime initializing" << std::endl;
    std::cout << "database path: " << config_.databasePath() << std::endl;

    if (!database_.open(config_.databasePath())) {
        std::cerr << "failed to open database" << std::endl;
        return false;
    }

    std::cout << "database opened" << std::endl;

    jobRepository_ = std::make_unique<JobRepository>(database_);
    recordingRepository_ = std::make_unique<RecordingRepository>(database_);
    metadataRepository_ = std::make_unique<MetadataRepository>(database_);
    jobDashboardService_ = std::make_unique<JobDashboardService>(*jobRepository_);
    recordingDashboardService_ = std::make_unique<RecordingDashboardService>(*recordingRepository_, *metadataRepository_);
    dashboardFacade_ = std::make_unique<DashboardFacade>(*jobDashboardService_, *recordingDashboardService_);
    dashboardJsonSerializer_ = std::make_unique<DashboardJsonSerializer>();
    dashboardController_ = std::make_unique<DashboardController>(*dashboardFacade_, *dashboardJsonSerializer_);
    jobsController_ = std::make_unique<JobsController>(*jobRepository_);
    recordingsController_ = std::make_unique<RecordingsController>(*recordingRepository_);
    metadataController_ = std::make_unique<MetadataController>(*metadataRepository_);

    std::cout << "REST controller runtime initialized" << std::endl;

    BackendNode defaultBackend;
    defaultBackend.backendId = "default";
    defaultBackend.backendName = "Default VDR";
    defaultBackend.backendType = "vdr";
    defaultBackend.connection.enabled = true;
    defaultBackend.connection.mode = config_.vdrMode();
    defaultBackend.connection.host = config_.vdrHost();
    defaultBackend.connection.port = config_.vdrPort();
    defaultBackend.enabled = true;
    defaultBackend.online = false;

    backendRegistry_.addBackend(defaultBackend);
    backendRegistryService_ = std::make_unique<BackendRegistryService>(backendRegistry_);
    backendRegistryJsonSerializer_ = std::make_unique<BackendRegistryJsonSerializer>();
    backendRegistryController_ = std::make_unique<BackendRegistryController>(*backendRegistryService_, *backendRegistryJsonSerializer_);

    const auto runtimeBackend =
        backendRegistry_.getBackend("default");

    if (!runtimeBackend.has_value()) {
        std::cerr << "failed to initialize default backend" << std::endl;
        return false;
    }

    vdrConfig_ = runtimeBackend->connection;

    snapshotCache_ = std::make_unique<SnapshotCache>();
    snapshotCacheService_ = std::make_unique<SnapshotCacheService>(*snapshotCache_);
    snapshotAccessService_ = std::make_unique<SnapshotAccessService>(*snapshotCacheService_);
    vdrSnapshotReadService_ = std::make_unique<VdrSnapshotReadService>(*snapshotAccessService_);
    vdrSnapshotReadJsonSerializer_ = std::make_unique<VdrSnapshotReadJsonSerializer>();

    defaultBackendContext_ = std::make_unique<BackendRuntimeContext>();
    defaultBackendContext_->backendId = runtimeBackend->backendId;
    defaultBackendContext_->httpClient = std::make_unique<BasicHttpClient>(vdrConfig_.host, vdrConfig_.port, &runtimeLogger_, &runtimeDiagnosticsService_);
    defaultBackendContext_->adapter = std::make_unique<RestfulApiVdrAdapter>(vdrConfig_, *defaultBackendContext_->httpClient);
    defaultBackendContext_->service = std::make_unique<VdrService>(*defaultBackendContext_->adapter, &runtimeLogger_);
    defaultBackendContext_->snapshotBuilder = std::make_unique<VdrSnapshotBuilder>(
        *defaultBackendContext_->service,
        defaultBackendContext_->backendId,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);
    defaultBackendContext_->pollingService = std::make_unique<PollingService>(
        *defaultBackendContext_->snapshotBuilder,
        *defaultBackendContext_->service,
        *snapshotCacheService_,
        defaultBackendContext_->backendId,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);
    snapshotChangeFeed_ = std::make_unique<SnapshotChangeFeed>();
    snapshotChangeFeedService_ = std::make_unique<SnapshotChangeFeedService>();
    snapshotChangeFeedJsonSerializer_ = std::make_unique<SnapshotChangeFeedJsonSerializer>();

    pollVdrAndUpdateChangeFeed();

    std::cout << "VDR snapshot runtime initialized" << std::endl;

    vdrOverviewService_ = std::make_unique<VdrOverviewService>(*snapshotAccessService_);
    vdrOverviewJsonSerializer_ = std::make_unique<VdrOverviewJsonSerializer>();
    vdrController_ = std::make_unique<VdrController>(*vdrOverviewService_, *vdrOverviewJsonSerializer_, *vdrSnapshotReadService_, *vdrSnapshotReadJsonSerializer_);

    std::cout << "VDR controller runtime initialized" << std::endl;

    runtimeDiagnosticsJsonSerializer_ = std::make_unique<RuntimeDiagnosticsJsonSerializer>();
    runtimeDiagnosticsController_ = std::make_unique<RuntimeDiagnosticsController>(runtimeDiagnosticsService_, *runtimeDiagnosticsJsonSerializer_);
    snapshotChangeFeedController_ = std::make_unique<SnapshotChangeFeedController>(*snapshotChangeFeed_, *snapshotChangeFeedJsonSerializer_);

    std::cout << "runtime diagnostics controller initialized" << std::endl;
    std::cout << "snapshot change feed controller initialized" << std::endl;

    apiRouter_ = std::make_unique<ApiRouter>(*dashboardController_, *jobsController_, *recordingsController_, *metadataController_, *vdrController_, *backendRegistryController_, *runtimeDiagnosticsController_, *snapshotChangeFeedController_);

    std::cout << "API router runtime initialized" << std::endl;

    httpServer_ = std::make_unique<TestHttpServer>(*apiRouter_);
    httpListener_ = std::make_unique<SimpleHttpListener>(config_.httpListenHost(), config_.httpListenPort(), *httpServer_);

    std::cout << "HTTP listener runtime initialized" << std::endl;
    std::cout << "dashboard runtime initialized" << std::endl;

    std::signal(SIGINT, DaemonRuntime::handleSignal);
    std::signal(SIGTERM, DaemonRuntime::handleSignal);

    shutdownRequested_ = false;
    initialized_ = true;

    return true;
}

void DaemonRuntime::pollVdrAndUpdateChangeFeed()
{
    defaultBackendContext_->pollingService->poll();

    snapshotChangeFeedService_->appendChanges(
        *snapshotChangeFeed_,
        snapshotCacheService_->generation(),
        defaultBackendContext_->pollingService->changeEvents());
}

int DaemonRuntime::run()
{
    if (!initialized_) {
        std::cerr << "vdr-suite-daemon runtime not initialized" << std::endl;
        return 1;
    }

    std::cout << "vdr-suite-daemon runtime running" << std::endl;
    std::cout << "vdr-suite-daemon serving HTTP on " << config_.httpListenHost() << ":" << config_.httpListenPort() << std::endl;

    return httpListener_->runUntilStopped();
}

void DaemonRuntime::shutdown()
{
    if (!initialized_) {
        return;
    }

    httpListener_.reset();
    httpServer_.reset();
    apiRouter_.reset();

    std::cout << "HTTP server runtime stopped" << std::endl;

    snapshotChangeFeedController_.reset();
    runtimeDiagnosticsController_.reset();
    backendRegistryController_.reset();
    backendRegistryJsonSerializer_.reset();
    backendRegistryService_.reset();
    runtimeDiagnosticsJsonSerializer_.reset();
    vdrController_.reset();
    vdrOverviewJsonSerializer_.reset();
    vdrOverviewService_.reset();
    snapshotChangeFeedJsonSerializer_.reset();
    snapshotChangeFeedService_.reset();
    snapshotChangeFeed_.reset();
    defaultBackendContext_.reset();
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
