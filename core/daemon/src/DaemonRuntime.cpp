#include "DaemonRuntime.h"

#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "RestfulApiVdrTimerActionExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"
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

    if (vdrTimerActionExecutorAdapterRegistry_) {
        vdrTimerActionExecutorAdapterRegistry_->registerAdapter(
            std::make_shared<RestfulApiVdrTimerActionExecutorAdapter>(
                context->backendId,
                "",
                *context->httpClient));
    }
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
    defaultBackend.backendType = "restfulapi";
    defaultBackend.connection.enabled = true;
    defaultBackend.connection.mode = config_.vdrMode();
    defaultBackend.connection.host = config_.vdrHost();
    defaultBackend.connection.port = config_.vdrPort();
    defaultBackend.enabled = true;
    defaultBackend.online = true;

    backendRegistry_.addBackend(defaultBackend);
    backendRegistryService_ = std::make_unique<BackendRegistryService>(backendRegistry_);
    backendRegistryJsonSerializer_ = std::make_unique<BackendRegistryJsonSerializer>();
    backendRegistryController_ = std::make_unique<BackendRegistryController>(*backendRegistryService_, *backendRegistryJsonSerializer_);

    const auto runtimeBackends =
        backendRegistry_.listBackends();

    if (runtimeBackends.empty()) {
        std::cout << "no VDR backends configured" << std::endl;
    }

    snapshotCache_ = std::make_unique<SnapshotCache>();
    snapshotCacheService_ = std::make_unique<SnapshotCacheService>(*snapshotCache_);
    snapshotAccessService_ = std::make_unique<SnapshotAccessService>(*snapshotCacheService_);
    vdrSnapshotReadService_ = std::make_unique<VdrSnapshotReadService>(*snapshotAccessService_);
    vdrSnapshotReadJsonSerializer_ = std::make_unique<VdrSnapshotReadJsonSerializer>();

    backendPollingCoordinator_ = std::make_unique<BackendPollingCoordinator>();
    vdrTimerActionExecutorAdapterRegistry_ = std::make_unique<VdrTimerActionExecutorAdapterRegistry>();

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

    if (!backendRuntimeContexts_.empty()) {
        pollVdrAndUpdateChangeFeed();
    }

    std::cout << "VDR snapshot runtime initialized" << std::endl;

    vdrOverviewService_ = std::make_unique<VdrOverviewService>(*snapshotAccessService_);
    vdrOverviewJsonSerializer_ = std::make_unique<VdrOverviewJsonSerializer>();
    vdrController_ = std::make_unique<VdrController>(*vdrOverviewService_, *vdrOverviewJsonSerializer_, *vdrSnapshotReadService_, *vdrSnapshotReadJsonSerializer_);

    if (!backendRuntimeContexts_.empty()) {
        vdrRecordingQueryService_ = std::make_unique<VdrRecordingQueryService>(
            *backendRuntimeContexts_.front()->service);
    }
    else {
        std::cerr << "failed to initialize VDR recording query controller: no VDR backend configured" << std::endl;
        return false;
    }

    vdrRecordingQueryResultJsonSerializer_ = std::make_unique<VdrRecordingQueryResultJsonSerializer>();
    vdrRecordingQueryController_ = std::make_unique<VdrRecordingQueryController>(
        *vdrRecordingQueryService_,
        *vdrRecordingQueryResultJsonSerializer_);

    capabilitySet_ = std::make_unique<VdrCapabilitySet>(
        VdrCapabilitySet::snapshotReadOnly());
    capabilityResolver_ = std::make_unique<CapabilityResolver>(*capabilitySet_);
    capabilityReportBuilder_ = std::make_unique<CapabilityReportBuilder>();
    capabilityReportService_ = std::make_unique<CapabilityReportService>(
        "default",
        *capabilityResolver_,
        *capabilityReportBuilder_);
    capabilityReportJsonSerializer_ = std::make_unique<CapabilityReportJsonSerializer>();
    capabilityController_ = std::make_unique<CapabilityController>(
        *capabilityReportService_,
        *capabilityReportJsonSerializer_);

    std::cout << "capability controller runtime initialized" << std::endl;

    if (!backendRuntimeContexts_.empty()) {
        epgQueryService_ = std::make_unique<EpgQueryService>(
            *backendRuntimeContexts_.front()->service);
        epgController_ = std::make_unique<EpgController>(*epgQueryService_);

        std::cout << "EPG controller runtime initialized" << std::endl;
    }
    else {
        std::cout << "EPG controller runtime skipped: no VDR backend configured" << std::endl;
    }

    std::cout << "VDR controller runtime initialized" << std::endl;

    recordingActionValidationService_ = std::make_unique<RecordingActionValidationService>();
    recordingActionValidationResultJsonSerializer_ = std::make_unique<RecordingActionValidationResultJsonSerializer>();
    recordingActionValidationRequestParser_ = std::make_unique<RecordingActionValidationRequestParser>();
    recordingActionValidationController_ = std::make_unique<RecordingActionValidationController>(
        *recordingActionValidationService_,
        *recordingActionValidationResultJsonSerializer_,
        *recordingActionValidationRequestParser_);

    recordingActionExecutionService_ = std::make_unique<RecordingActionExecutionService>();
    recordingActionExecutionResultJsonSerializer_ = std::make_unique<RecordingActionExecutionResultJsonSerializer>();
    recordingActionBackendExecutorAdapterRegistry_ = std::make_unique<RecordingActionBackendExecutorAdapterRegistry>();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        RestfulApiRecordingActionBackendConfig recordingActionConfig;
        recordingActionConfig.backendId = backendRuntimeContext->backendId;
        recordingActionConfig.basePath = "";
        recordingActionConfig.allowExecution = true;
        recordingActionConfig.readOnly = false;

        recordingActionBackendExecutorAdapterRegistry_->registerAdapter(
            std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
                recordingActionConfig,
                *backendRuntimeContext->httpClient));
    }

    recordingActionExecutionController_ = std::make_unique<RecordingActionExecutionController>(
        *recordingActionExecutionService_,
        *recordingActionExecutionResultJsonSerializer_,
        *recordingActionBackendExecutorAdapterRegistry_,
        backendRegistry_,
        *recordingActionValidationRequestParser_,
        *vdrSnapshotReadService_);
    recordingActionExecutionController_->setAfterSuccessfulExecutionCallback(
        [this]() {
            pollVdrAndUpdateChangeFeed();
        });

    vdrTimerActionService_ = std::make_unique<VdrTimerActionService>();
    vdrTimerActionExecutionService_ = std::make_unique<VdrTimerActionExecutionService>();
    vdrTimerActionResultJsonSerializer_ = std::make_unique<VdrTimerActionResultJsonSerializer>();
    vdrTimerActionRequestParser_ = std::make_unique<VdrTimerActionRequestParser>();
    vdrTimerActionController_ = std::make_unique<VdrTimerActionController>(
        *vdrTimerActionExecutionService_,
        *vdrTimerActionResultJsonSerializer_,
        *vdrTimerActionRequestParser_);

    std::cout << "recording action controller runtime initialized" << std::endl;
    std::cout << "VDR timer action controller runtime initialized" << std::endl;

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

    apiRouter_ = std::make_unique<ApiRouter>(
        *dashboardController_,
        *jobsController_,
        *recordingsController_,
        *metadataController_,
        *vdrController_,
        *vdrRecordingQueryController_,
        epgController_.get(),
        *backendRegistryController_,
        *capabilityController_,
        *recordingActionValidationController_,
        *recordingActionExecutionController_,
        *vdrTimerActionController_,
        *vdrTimerActionExecutorAdapterRegistry_,
        *runtimeDiagnosticsController_,
        *snapshotChangeFeedController_,
        *liveTransportController_);

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
    vdrTimerActionController_.reset();
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
    backendRegistryService_.reset();
    runtimeDiagnosticsJsonSerializer_.reset();
    vdrRecordingQueryController_.reset();
    vdrRecordingQueryResultJsonSerializer_.reset();
    vdrRecordingQueryService_.reset();
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
