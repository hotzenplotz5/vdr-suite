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


std::unique_ptr<BackendRuntimeContext> DaemonRuntime::createBackendRuntimeContext(
    const BackendNode& backend)
{
    VdrConfig backendConfig = backend.connection;

    auto context = std::make_unique<BackendRuntimeContext>();

    context->backendId = backend.backendId;
    context->httpClient = std::make_unique<BasicHttpClient>(
        backendConfig.host,
        backendConfig.port,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);
    context->adapter = std::make_unique<RestfulApiVdrAdapter>(
        backendConfig,
        *context->httpClient);
    context->service = std::make_unique<VdrService>(
        *context->adapter,
        &runtimeLogger_);
    context->snapshotBuilder = std::make_unique<VdrSnapshotBuilder>(
        *context->service,
        context->backendId,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);
    context->pollingService = std::make_unique<PollingService>(
        *context->snapshotBuilder,
        *context->service,
        *snapshotCacheService_,
        context->backendId,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);

    return context;
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

    const auto runtimeBackends =
        backendRegistry_.listBackends();

    if (runtimeBackends.empty()) {
        std::cerr << "failed to initialize VDR backends" << std::endl;
        return false;
    }

    snapshotCache_ = std::make_unique<SnapshotCache>();
    snapshotCacheService_ = std::make_unique<SnapshotCacheService>(*snapshotCache_);
    snapshotAccessService_ = std::make_unique<SnapshotAccessService>(*snapshotCacheService_);
    vdrSnapshotReadService_ = std::make_unique<VdrSnapshotReadService>(*snapshotAccessService_);
    vdrSnapshotReadJsonSerializer_ = std::make_unique<VdrSnapshotReadJsonSerializer>();

    backendPollingCoordinator_ = std::make_unique<BackendPollingCoordinator>();

    for (const BackendNode& runtimeBackend : runtimeBackends) {
        auto backendRuntimeContext =
            createBackendRuntimeContext(runtimeBackend);

        backendPollingCoordinator_->addPollingService(
            backendRuntimeContext->backendId,
            *backendRuntimeContext->pollingService);

        backendRuntimeContexts_.push_back(
            std::move(backendRuntimeContext));
    }

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

    liveTransport_ = std::make_unique<SseLiveTransport>();
    liveTransportService_ = std::make_unique<LiveTransportService>(*liveTransport_);
    liveTransportController_ = std::make_unique<LiveTransportController>(*liveTransport_);

    std::cout << "runtime diagnostics controller initialized" << std::endl;
    std::cout << "snapshot change feed controller initialized" << std::endl;
    std::cout << "live transport service initialized" << std::endl;
    std::cout << "live transport controller initialized" << std::endl;

    apiRouter_ = std::make_unique<ApiRouter>(*dashboardController_, *jobsController_, *recordingsController_, *metadataController_, *vdrController_, *backendRegistryController_, *runtimeDiagnosticsController_, *snapshotChangeFeedController_, *liveTransportController_);

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
    backendPollingCoordinator_->pollAll();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        const int previousLatestSequenceNumber =
            snapshotChangeFeed_->latestSequenceNumber();

        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_,
            snapshotCacheService_->generation(),
            backendRuntimeContext->pollingService->changeEvents(),
            backendRuntimeContext->backendId);

        for (const auto& entry : snapshotChangeFeed_->entries()) {
            if (entry.sequenceNumber() > previousLatestSequenceNumber) {
                liveTransportService_->publishChangeFeedEntry(entry);
            }
        }
    }
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

    liveTransportController_.reset();
    liveTransportService_.reset();
    liveTransport_.reset();
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
    backendPollingCoordinator_.reset();
    backendRuntimeContexts_.clear();
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
