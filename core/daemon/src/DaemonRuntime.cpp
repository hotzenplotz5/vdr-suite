#include "DaemonRuntime.h"

#include "BasicHttpClient.h"
#include "RestfulApiVdrAdapter.h"
#include "RestfulApiEventStreamClient.h"
#include "RestfulApiVdrTimerActionExecutorAdapter.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"
#include "RestfulApiSearchTimerAdapter.h"
#include "RestfulApiSearchTimerDiscoveryProvider.h"
#include "SearchTimerDiscoveryStaticProvider.h"
#include "SimpleHttpListener.h"
#include "TestHttpServer.h"
#include "RecordingArtworkHttpServer.h"
#include "EpgSearchNativeFuzzyStartupRestoreDiagnostics.h"
#include "VdrEventQuery.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingNativePersonSearchService.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <csignal>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>

namespace
{
std::int64_t recordingMetadataEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool recordingMetadataCapabilityAvailable(
    const BackendRuntimeContext& context)
{
    if (!context.suiteBridgeAgentRuntime) {
        return false;
    }

    const vdrsuite::agent::SuiteBridgeEmbeddedAgentHealth health =
        context.suiteBridgeAgentRuntime->health();

    return health.running &&
        health.observation.hasDiscovery &&
        health.observation.discovery.capabilityAvailable(
            "recording-metadata");
}

void runRecordingMetadataEnrichment(
    BackendRuntimeContext& context,
    const std::vector<VdrRecording>& recordings,
    std::atomic<bool>& stopRequested,
    const std::string& reason)
{
    if (!context.recordingMetadataEnrichmentService) {
        return;
    }

    const std::int64_t now = recordingMetadataEpochSeconds();

    const std::size_t queued =
        context.recordingMetadataEnrichmentService->reconcileInventory(
            recordings,
            now);

    if (stopRequested.load() || queued == 0) {
        return;
    }

    if (!recordingMetadataCapabilityAvailable(context)) {
        if (reason != "periodic") {
            std::cout
                << "Recording metadata enrichment deferred: reason="
                << reason
                << ", backend="
                << context.backendId
                << ", queued="
                << queued
                << ", capability=unavailable"
                << std::endl;
        }

        return;
    }

    const int processed =
        context.recordingMetadataEnrichmentService->processBatch(now);

    const VdrRecordingNativeMetadataEnrichmentStatus status =
        context.recordingMetadataEnrichmentService->status();

    std::cout
        << "Recording metadata enrichment finished: reason="
        << reason
        << ", backend="
        << context.backendId
        << ", queued="
        << queued
        << ", processed="
        << processed
        << ", remaining="
        << status.queuedCount
        << std::endl;
}
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

    context->searchTimerAdapter = std::make_unique<RestfulApiSearchTimerAdapter>(
        context->backendId,
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

    if (searchTimerPreviewEpgCache_) {
        context->searchTimerPreviewEpgCacheRefreshService =
            std::make_unique<SearchTimerPreviewEpgCacheRefreshService>(
                *searchTimerPreviewEpgCache_,
                *context->snapshotBuilder);
    }

    context->pollingService = std::make_unique<PollingService>(
        *context->snapshotBuilder,
        *context->service,
        *snapshotCacheService_,
        context->backendId,
        &runtimeLogger_,
        &runtimeDiagnosticsService_);

    context->eventStreamClient = std::make_unique<RestfulApiEventStreamClient>(
        context->backendId,
        backendConfig.host,
        backendConfig.port + 1,
        [this](const std::string&) {
            externalVdrChangeHint_.store(true);
            epgCacheDirtyHint_.store(true);
            recordingCacheDirtyHint_.store(true);
        });

    const RuntimeSuiteBridgeConfig& suiteBridgeConfig =
        config_.suiteBridge();

    if (suiteBridgeConfig.enabled &&
        context->backendId == suiteBridgeConfig.backendId) {
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig transportConfig;
        transportConfig.host = suiteBridgeConfig.host;
        transportConfig.port = suiteBridgeConfig.port;
        transportConfig.connectTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.connectTimeoutMs);
        transportConfig.ioTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.ioTimeoutMs);
        transportConfig.operationTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.operationTimeoutMs);

        context->suiteBridgeTransport =
            std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(
                std::move(transportConfig));

        if (epgArtworkRepository_) {
            context->epgArtworkResolver =
                std::make_unique<SuiteBridgeEpgArtworkResolver>(
                    *context->suiteBridgeTransport);
            context->epgScraperMetadataResolver =
                std::make_unique<SuiteBridgeEpgMetadataResolver>(
                    *context->suiteBridgeTransport);
            context->epgArtworkEnrichmentService =
                std::make_unique<EpgArtworkEnrichmentService>(
                    *epgArtworkRepository_,
                    *context->epgArtworkResolver);
        }

        context->recordingMetadataRepository =
            std::make_unique<VdrRecordingNativeMetadataRepository>(
                database_);

        if (!context->recordingMetadataRepository->ensureSchema()) {
            std::cerr
                << "failed to initialize native recording metadata schema: backend="
                << context->backendId
                << std::endl;

            context->recordingMetadataRepository.reset();
        }
        else {
            context->recordingMetadataResolver =
                std::make_unique<SuiteBridgeRecordingMetadataResolver>(
                    *context->suiteBridgeTransport);

            VdrRecordingNativeMetadataEnrichmentConfig enrichmentConfig;
            enrichmentConfig.maximumQueuedRecordings = 64;
            enrichmentConfig.maximumBatchSize = 8;

            context->recordingMetadataEnrichmentService =
                std::make_unique<VdrRecordingNativeMetadataEnrichmentService>(
                    context->backendId,
                    *context->recordingMetadataRepository,
                    *context->recordingMetadataResolver,
                    enrichmentConfig);
        }

        vdrsuite::agent::SuiteBridgeEmbeddedAgentConfig embeddedConfig;
        embeddedConfig.backendId = context->backendId;
        embeddedConfig.enabled = true;
        embeddedConfig.transport.host = suiteBridgeConfig.host;
        embeddedConfig.transport.port = suiteBridgeConfig.port;
        embeddedConfig.transport.connectTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.connectTimeoutMs);
        embeddedConfig.transport.ioTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.ioTimeoutMs);
        embeddedConfig.transport.operationTimeout =
            std::chrono::milliseconds(suiteBridgeConfig.operationTimeoutMs);
        embeddedConfig.observation.pollInterval =
            std::chrono::milliseconds(suiteBridgeConfig.pollIntervalMs);
        embeddedConfig.observation.staleAfter =
            std::chrono::milliseconds(suiteBridgeConfig.staleAfterMs);
        embeddedConfig.observation.offlineAfter =
            std::chrono::milliseconds(suiteBridgeConfig.offlineAfterMs);
        embeddedConfig.observation.reconnectInitial =
            std::chrono::milliseconds(suiteBridgeConfig.reconnectInitialMs);
        embeddedConfig.observation.reconnectMaximum =
            std::chrono::milliseconds(suiteBridgeConfig.reconnectMaximumMs);

        context->suiteBridgeAgentRuntime =
            std::make_unique<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime>(
                std::move(embeddedConfig));
    }

    if (epgEventRepository_) {
        context->epgCacheService = std::make_unique<EpgCacheService>(
            *epgEventRepository_,
            *context->service,
            context->epgArtworkEnrichmentService.get());
    }

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
    defaultBackend.capabilities = VdrCapabilitySet::snapshotReadOnly();

    backendRegistry_.addBackend(defaultBackend);
    backendRegistryService_ = std::make_unique<BackendRegistryService>(backendRegistry_);
    backendAccessPolicy_ = std::make_unique<BackendAccessPolicy>();
    backendRegistryJsonSerializer_ = std::make_unique<BackendRegistryJsonSerializer>();
    backendRegistryController_ = std::make_unique<BackendRegistryController>(*backendRegistryService_, *backendRegistryJsonSerializer_);

    epgSearchNativeFuzzyCapabilityRepository_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityRepository>(
            database_);
    epgSearchNativeFuzzyCapabilityDetector_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityDetector>();
    epgSearchNativeFuzzyCapabilityFreshnessPolicy_ =
        std::make_unique<EpgSearchNativeFuzzyCapabilityFreshnessPolicy>();
    epgSearchNativeFuzzyStartupRestoreService_ =
        std::make_unique<EpgSearchNativeFuzzyStartupRestoreService>(
            *epgSearchNativeFuzzyCapabilityRepository_,
            *epgSearchNativeFuzzyCapabilityDetector_,
            *backendRegistryService_,
            *epgSearchNativeFuzzyCapabilityFreshnessPolicy_);

    const auto nativeFuzzyStartupRestoreSummary =
        epgSearchNativeFuzzyStartupRestoreService_->restoreAllBackends();
    const auto nativeFuzzyStartupRestoreDiagnostics =
        EpgSearchNativeFuzzyStartupRestoreDiagnostics::fromSummary(
            nativeFuzzyStartupRestoreSummary);

    runtimeDiagnosticsService_.recordMeasurement(
        nativeFuzzyStartupRestoreDiagnostics.toRuntimeMeasurement());

    if (nativeFuzzyStartupRestoreDiagnostics.schemaReady) {
        std::cout
            << "EPGSearch native fuzzy persisted capability restore: "
            << "status=" << nativeFuzzyStartupRestoreDiagnostics.status()
            << ", reason=\"" << nativeFuzzyStartupRestoreDiagnostics.reason() << "\""
            << ", backends=" << nativeFuzzyStartupRestoreDiagnostics.backendsSeen
            << ", persisted=" << nativeFuzzyStartupRestoreDiagnostics.persistedResultsFound
            << ", updated=" << nativeFuzzyStartupRestoreDiagnostics.backendsUpdated
            << ", nativeAvailable=" << nativeFuzzyStartupRestoreDiagnostics.nativeFuzzyAvailable
            << ", nativeUnavailable=" << nativeFuzzyStartupRestoreDiagnostics.nativeFuzzyUnavailable
            << ", staleIgnored=" << nativeFuzzyStartupRestoreDiagnostics.staleResultsIgnored
            << std::endl;
    }
    else {
        std::cerr
            << "EPGSearch native fuzzy persisted capability restore skipped: "
            << nativeFuzzyStartupRestoreDiagnostics.reason()
            << std::endl;
    }
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
    searchTimerPreviewEpgCache_ = std::make_unique<SearchTimerPreviewEpgCache>();
    searchTimerPreviewEpgCacheRefreshServiceRegistry_ =
        std::make_unique<SearchTimerPreviewEpgCacheRefreshServiceRegistry>();
    epgEventRepository_ = std::make_unique<EpgEventRepository>(database_);

    if (!epgEventRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG cache repository schema" << std::endl;
        return false;
    }

    epgArtworkRepository_ = std::make_unique<EpgArtworkRepository>(database_);

    if (!epgArtworkRepository_->ensureSchema()) {
        std::cerr << "failed to initialize EPG artwork repository schema" << std::endl;
        return false;
    }

    epgArtworkPublicJsonSerializer_ =
        std::make_unique<EpgArtworkPublicJsonSerializer>();

    vdrRecordingCacheRepository_ = std::make_unique<VdrRecordingCacheRepository>(database_);

    if (!vdrRecordingCacheRepository_->ensureSchema()) {
        std::cerr << "failed to initialize VDR recording cache repository schema" << std::endl;
        return false;
    }

    epgCacheServiceRegistry_ = std::make_unique<EpgCacheServiceRegistry>();

    for (const BackendNode& runtimeBackend : runtimeBackends) {
        auto backendRuntimeContext =
            createBackendRuntimeContext(runtimeBackend);

        if (backendRuntimeContext->searchTimerPreviewEpgCacheRefreshService) {
            searchTimerPreviewEpgCacheRefreshServiceRegistry_->registerService(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->searchTimerPreviewEpgCacheRefreshService);
        }

        if (backendRuntimeContext->epgCacheService) {
            epgCacheServiceRegistry_->registerService(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->epgCacheService);
        }

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
    vdrController_ = std::make_unique<VdrController>(
        *vdrOverviewService_,
        *vdrOverviewJsonSerializer_,
        *vdrSnapshotReadService_,
        *vdrSnapshotReadJsonSerializer_,
        backendRuntimeContexts_.empty()
            ? nullptr
            : backendRuntimeContexts_.front()->service.get());

    if (!backendRuntimeContexts_.empty()) {
        vdrRecordingQueryService_ = std::make_unique<VdrRecordingQueryService>(
            *backendRuntimeContexts_.front()->service,
            vdrRecordingCacheRepository_.get(),
            backendRuntimeContexts_.front()->backendId);
    }
    else {
        std::cerr << "failed to initialize VDR recording query controller: no VDR backend configured" << std::endl;
        return false;
    }

    vdrRecordingQueryResultJsonSerializer_ = std::make_unique<VdrRecordingQueryResultJsonSerializer>();
    vdrRecordingQueryController_ = std::make_unique<VdrRecordingQueryController>(
        *vdrRecordingQueryService_,
        *vdrRecordingQueryResultJsonSerializer_);
    vdrRecordingFolderController_ =
        std::make_unique<VdrRecordingFolderController>(
            *vdrRecordingCacheRepository_,
            [this](
                const std::string& backendId,
                const std::string& backendNativeId)
            {
                const std::string normalizedBackendId =
                    backendId.empty()
                        ? "default"
                        : backendId;

                for (const auto& backendRuntimeContext :
                     backendRuntimeContexts_)
                {
                    if (!backendRuntimeContext ||
                        backendRuntimeContext->backendId !=
                            normalizedBackendId ||
                        !backendRuntimeContext
                             ->recordingMetadataRepository)
                    {
                        continue;
                    }

                    return backendRuntimeContext
                        ->recordingMetadataRepository
                        ->findByBackendNativeId(
                            normalizedBackendId,
                            backendNativeId);
                }

                return VdrRecordingNativeMetadataRecord{};
            });

    capabilityResolver_ =
        std::make_unique<BackendRegistryCapabilityResolver>(
            *backendRegistryService_,
            "default");
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
        epgSearchService_ = std::make_unique<EpgSearchService>();
        epgSearchResultJsonSerializer_ = std::make_unique<EpgSearchResultJsonSerializer>();
        epgController_ = std::make_unique<EpgController>(
            *epgQueryService_,
            *epgSearchService_,
            *epgSearchResultJsonSerializer_);

        std::cout << "EPG controller runtime initialized" << std::endl;
    }
    else {
        std::cout << "EPG controller runtime skipped: no VDR backend configured" << std::endl;
    }

    std::cout << "VDR controller runtime initialized" << std::endl;

    searchTimerService_ = std::make_unique<SearchTimerService>();
    searchTimerResultJsonSerializer_ = std::make_unique<SearchTimerResultJsonSerializer>();

    if (!backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->searchTimerAdapter) {
        searchTimerCreateService_ =
            std::make_unique<SearchTimerCreateService>();
        searchTimerCreateResultJsonSerializer_ =
            std::make_unique<SearchTimerCreateResultJsonSerializer>();
        searchTimerCreateRequestParser_ =
            std::make_unique<SearchTimerCreateRequestParser>();
        searchTimerUpdateService_ =
            std::make_unique<SearchTimerUpdateService>();
        searchTimerUpdateResultJsonSerializer_ =
            std::make_unique<SearchTimerUpdateResultJsonSerializer>();
        searchTimerUpdateRequestParser_ =
            std::make_unique<SearchTimerUpdateRequestParser>();
        searchTimerDeleteService_ =
            std::make_unique<SearchTimerDeleteService>();
        searchTimerDeleteResultJsonSerializer_ =
            std::make_unique<SearchTimerDeleteResultJsonSerializer>();
        searchTimerDeleteRequestParser_ =
            std::make_unique<SearchTimerDeleteRequestParser>();

        searchTimerCommandExecutor_ =
            std::make_unique<RestfulApiSearchTimerCommandExecutor>(
                *backendRuntimeContexts_.front()->httpClient);
        searchTimerRuntimeMutationPolicyExecutor_ =
            std::make_unique<SearchTimerRuntimeMutationPolicyExecutor>(
                *searchTimerCommandExecutor_,
                false);

        searchTimerController_ = std::make_unique<SearchTimerController>(
            *searchTimerService_,
            *searchTimerResultJsonSerializer_,
            *backendRuntimeContexts_.front()->searchTimerAdapter,
            *searchTimerCreateService_,
            *searchTimerCreateResultJsonSerializer_,
            *searchTimerCreateRequestParser_,
            searchTimerUpdateService_.get(),
            searchTimerUpdateResultJsonSerializer_.get(),
            searchTimerUpdateRequestParser_.get(),
            searchTimerDeleteService_.get(),
            searchTimerDeleteResultJsonSerializer_.get(),
            searchTimerDeleteRequestParser_.get());

        std::cout << "SearchTimer controller runtime initialized" << std::endl;
        std::cout << "SearchTimer command executor runtime initialized" << std::endl;
        std::cout << "SearchTimer runtime mutation policy executor initialized closed" << std::endl;
    }
    else {
        std::cout << "SearchTimer controller runtime skipped: no VDR backend configured" << std::endl;
    }

    if (!backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->httpClient) {
        searchTimerDiscoveryProvider_ =
            std::make_unique<RestfulApiSearchTimerDiscoveryProvider>(
                backendRuntimeContexts_.front()->backendId,
                *backendRuntimeContexts_.front()->httpClient,
                "");
        std::cout << "SearchTimer discovery provider runtime initialized: restfulapi" << std::endl;
    }
    else {
        searchTimerDiscoveryProvider_ =
            std::make_unique<SearchTimerDiscoveryStaticProvider>();
        std::cout << "SearchTimer discovery provider runtime initialized: static fallback" << std::endl;
    }

    searchTimerDiscoveryService_ =
        std::make_unique<SearchTimerDiscoveryService>(
            *searchTimerDiscoveryProvider_);
    searchTimerDiscoveryJsonSerializer_ =
        std::make_unique<SearchTimerDiscoveryJsonSerializer>();
    searchTimerDiscoveryController_ =
        std::make_unique<SearchTimerDiscoveryController>(
            *searchTimerDiscoveryService_,
            *searchTimerDiscoveryJsonSerializer_);

    std::cout << "SearchTimer discovery controller runtime initialized" << std::endl;

    searchTimerAutomationReadOnlyService_ =
        std::make_unique<SearchTimerAutomationReadOnlyService>();
    searchTimerAutomationDryRunResultJsonSerializer_ =
        std::make_unique<SearchTimerAutomationDryRunResultJsonSerializer>();
    searchTimerAutomationPreviewController_ =
        std::make_unique<SearchTimerAutomationPreviewController>(
            *searchTimerAutomationReadOnlyService_,
            *searchTimerAutomationDryRunResultJsonSerializer_);

    std::cout << "SearchTimer automation preview controller runtime initialized" << std::endl;

    searchTimerPreviewEpgCacheRefreshController_ =
        std::make_unique<SearchTimerPreviewEpgCacheRefreshController>(
            *searchTimerPreviewEpgCacheRefreshServiceRegistry_);

    std::cout << "SearchTimer preview EPG cache refresh controller runtime initialized" << std::endl;

    epgCacheController_ = std::make_unique<EpgCacheController>(
        *epgCacheServiceRegistry_,
        *epgArtworkRepository_,
        *epgArtworkPublicJsonSerializer_);

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (backendRuntimeContext &&
            backendRuntimeContext->epgScraperMetadataResolver) {
            epgCacheController_->registerScraperMetadataResolver(
                backendRuntimeContext->backendId,
                *backendRuntimeContext->epgScraperMetadataResolver);
        }
    }

    std::cout << "EPG cache controller runtime initialized" << std::endl;

    personResolutionJsonSerializer_ = std::make_unique<PersonResolutionJsonSerializer>();
    personSearchService_ = std::make_unique<PersonSearchService>();
    personQueryResultJsonSerializer_ = std::make_unique<PersonQueryResultJsonSerializer>();
    personController_ = std::make_unique<PersonController>(
        *personResolutionJsonSerializer_,
        *personSearchService_,
        *personQueryResultJsonSerializer_);

    std::cout << "person controller runtime initialized" << std::endl;

    recordingPersonSearchService_ =
        std::make_unique<RecordingPersonSearchService>();

    recordingPersonSearchResultJsonSerializer_ =
        std::make_unique<
            RecordingPersonSearchResultJsonSerializer>();

    std::vector<std::pair<
        std::string,
        VdrRecordingNativeMetadataRepository*>>
        recordingPersonMetadataRepositories;

    for (const auto& backendRuntimeContext :
         backendRuntimeContexts_)
    {
        if (!backendRuntimeContext ||
            !backendRuntimeContext
                 ->recordingMetadataRepository)
        {
            continue;
        }

        recordingPersonMetadataRepositories.emplace_back(
            backendRuntimeContext->backendId,
            backendRuntimeContext
                ->recordingMetadataRepository
                .get());
    }

    RecordingPersonSearchController::PersistentSearch
        persistentRecordingPersonSearch;

    if (!recordingPersonMetadataRepositories.empty())
    {
        VdrRecordingCacheRepository*
            recordingCacheRepository =
                vdrRecordingCacheRepository_.get();

        persistentRecordingPersonSearch =
            [
                recordingPersonMetadataRepositories =
                    std::move(
                        recordingPersonMetadataRepositories),
                recordingCacheRepository
            ](
                const std::string& backendId,
                const PersonQuery& query,
                int limit,
                int offset)
            {
                const std::string normalizedBackendId =
                    backendId.empty()
                        ? "default"
                        : backendId;

                for (const auto& repositoryEntry :
                     recordingPersonMetadataRepositories)
                {
                    if (repositoryEntry.first !=
                        normalizedBackendId)
                    {
                        continue;
                    }

                    VdrRecordingNativePersonSearchService
                        nativeSearchService(
                            *repositoryEntry.second,
                            *recordingCacheRepository);

                    return nativeSearchService.search(
                        normalizedBackendId,
                        query,
                        limit,
                        offset);
                }

                return RecordingPersonSearchResult::empty(
                    limit,
                    offset);
            };
    }

    const bool usesPersistentRecordingPersonSearch =
        static_cast<bool>(
            persistentRecordingPersonSearch);

    recordingPersonSearchController_ =
        std::make_unique<
            RecordingPersonSearchController>(
                *recordingPersonSearchService_,
                *recordingPersonSearchResultJsonSerializer_,
                std::move(
                    persistentRecordingPersonSearch));

    std::cout
        << "recording person search controller runtime initialized: "
        << "source="
        << (
            usesPersistentRecordingPersonSearch
                ? "native-persistent-index"
                : "snapshot-fallback"
        )
        << std::endl;

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
        recordingActionConfig.videoDirectory = "/srv/vdr/video";
        recordingActionConfig.apiMode =
            RestfulApiRecordingActionApiMode::SafeMutation;
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
        [this](const RecordingActionRequest& request) {
            const std::string backendId =
                request.backendId.empty()
                    ? "default"
                    : request.backendId;

            recordingCacheActionRefreshAttempts_.store(8);
            recordingCacheDirtyHint_.store(true);
            externalVdrChangeHint_.store(true);

            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                if (!backendRuntimeContext ||
                    backendRuntimeContext->backendId != backendId ||
                    !backendRuntimeContext->snapshotBuilder) {
                    continue;
                }

                const std::vector<VdrRecording> recordings =
                    backendRuntimeContext->snapshotBuilder->buildRecordings();

                snapshotCacheService_->updateRecordingsForBackend(
                    backendRuntimeContext->backendId,
                    recordings);

                if (vdrRecordingCacheRepository_) {
                    vdrRecordingCacheRepository_->replaceRecordingsForBackend(
                        backendRuntimeContext->backendId,
                        recordings);
                    vdrRecordingCacheRepository_->markRefreshFinished(
                        backendRuntimeContext->backendId,
                        static_cast<int>(recordings.size()));
                }

                break;
            }
        });

    recordingActionRequestPreviewService_ =
        std::make_unique<RecordingActionRequestPreviewService>();
    recordingActionRequestPreviewResultJsonSerializer_ =
        std::make_unique<RecordingActionRequestPreviewResultJsonSerializer>();
    recordingActionPreviewController_ =
        std::make_unique<RecordingActionPreviewController>(
            *recordingActionRequestPreviewService_,
            *recordingActionRequestPreviewResultJsonSerializer_,
            *recordingActionValidationRequestParser_);

    vdrTimerActionService_ = std::make_unique<VdrTimerActionService>();
    vdrTimerActionExecutionService_ = std::make_unique<VdrTimerActionExecutionService>();
    vdrTimerActionResultJsonSerializer_ = std::make_unique<VdrTimerActionResultJsonSerializer>();
    vdrTimerActionRequestParser_ = std::make_unique<VdrTimerActionRequestParser>();
    vdrTimerActionController_ = std::make_unique<VdrTimerActionController>(
        *vdrTimerActionExecutionService_,
        *vdrTimerActionResultJsonSerializer_,
        *vdrTimerActionRequestParser_,
        *backendRegistryService_,
        *backendAccessPolicy_);

    vdrChannelMoveExecutionService_ =
        std::make_unique<VdrChannelMoveExecutionService>();
    vdrChannelMoveResultJsonSerializer_ =
        std::make_unique<VdrChannelMoveResultJsonSerializer>();
    vdrChannelMoveRequestParser_ =
        std::make_unique<VdrChannelMoveRequestParser>();
    vdrChannelMoveExecutorAdapterRegistry_ =
        std::make_unique<VdrChannelMoveExecutorAdapterRegistry>();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (backendRuntimeContext &&
            backendRuntimeContext->backendId == "default") {
            vdrChannelMoveExecutorAdapterRegistry_->registerAdapter(
                std::make_shared<VdrChannelMoveExecutorAdapter>(
                    backendRuntimeContext->backendId,
                    std::make_shared<SvdrpChannelMoveExecutor>()));
        }
    }

    vdrChannelMoveController_ =
        std::make_unique<VdrChannelMoveController>(
            *vdrChannelMoveExecutionService_,
            *vdrChannelMoveResultJsonSerializer_,
            *vdrChannelMoveRequestParser_,
            *vdrChannelMoveExecutorAdapterRegistry_,
            *backendRegistryService_,
            *backendAccessPolicy_);

    vdrChannelMoveController_->setAfterSuccessfulMoveCallback(
        [this](const std::string& backendId) {
            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                if (backendRuntimeContext &&
                    backendRuntimeContext->backendId == backendId &&
                    backendRuntimeContext->snapshotBuilder) {
                    snapshotCacheService_->updateChannelsForBackend(
                        backendId,
                        backendRuntimeContext->snapshotBuilder->buildChannels());
                    externalVdrChangeHint_.store(true);
                    break;
                }
            }
        });

    std::cout << "recording action controller runtime initialized" << std::endl;
    std::cout << "VDR timer action controller runtime initialized" << std::endl;

    epgSearchNativeFuzzyStaleProbeAdministrationService_ =
        std::make_unique<EpgSearchNativeFuzzyStaleProbeAdministrationService>(
            *epgSearchNativeFuzzyCapabilityRepository_,
            *epgSearchNativeFuzzyCapabilityFreshnessPolicy_);
    epgSearchNativeFuzzyStaleProbeAdministrationController_ =
        std::make_unique<EpgSearchNativeFuzzyStaleProbeAdministrationController>(
            *epgSearchNativeFuzzyStaleProbeAdministrationService_);

    if (searchTimerCommandExecutor_ != nullptr
        && !backendRuntimeContexts_.empty()
        && backendRuntimeContexts_.front()->searchTimerAdapter)
    {
        epgSearchNativeFuzzyOperatorRefreshService_ =
            std::make_unique<EpgSearchNativeFuzzyOperatorRefreshService>(
                *searchTimerCommandExecutor_,
                *backendRuntimeContexts_.front()->searchTimerAdapter,
                *epgSearchNativeFuzzyCapabilityRepository_,
                *epgSearchNativeFuzzyCapabilityDetector_,
                *backendRegistryService_);
        epgSearchNativeFuzzyOperatorRefreshController_ =
            std::make_unique<EpgSearchNativeFuzzyOperatorRefreshController>(
                *epgSearchNativeFuzzyOperatorRefreshService_);
        std::cout << "EPGSearch native fuzzy operator refresh API runtime initialized" << std::endl;
    }
    else
    {
        std::cout << "EPGSearch native fuzzy operator refresh API runtime skipped: SearchTimer backend unavailable" << std::endl;
    }

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
        *vdrSnapshotReadService_,
        epgController_.get(),
        personController_.get(),
        recordingPersonSearchController_.get(),
        *backendRegistryController_,
        *capabilityController_,
        *recordingActionValidationController_,
        *recordingActionExecutionController_,
        *recordingActionPreviewController_,
        *vdrTimerActionController_,
        *vdrTimerActionExecutorAdapterRegistry_,
        *runtimeDiagnosticsController_,
        *snapshotChangeFeedController_,
        searchTimerController_.get(),
        *liveTransportController_,
        searchTimerRuntimeMutationPolicyExecutor_.get(),
        epgSearchNativeFuzzyStaleProbeAdministrationController_.get(),
        epgSearchNativeFuzzyOperatorRefreshController_.get(),
        searchTimerDiscoveryController_.get(),
        searchTimerAutomationPreviewController_.get(),
        searchTimerPreviewEpgCacheRefreshController_.get(),
        epgCacheController_.get(),
        vdrChannelMoveController_.get(),
        vdrRecordingFolderController_.get());

    apiRouter_->setSearchTimerPreviewEpgCache(
        searchTimerPreviewEpgCache_.get());

    std::cout << "API router runtime initialized" << std::endl;
    std::cout << "SearchTimer preview EPG cache runtime initialized" << std::endl;

    httpServer_ = std::make_unique<RecordingArtworkHttpServer>(
        std::make_unique<TestHttpServer>(*apiRouter_),
        *vdrRecordingCacheRepository_,
        config_.recordingArtworkRoots());

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
                std::chrono::duration_cast<std::chrono::seconds>(now - lastVdrPoll).count() < 5) {
                return;
            }

            lastVdrPoll = now;
            pollVdrAndUpdateChangeFeed();
        });

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (!backendRuntimeContext) {
            continue;
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            backendRuntimeContext->suiteBridgeAgentRuntime->start();
            std::cout
                << "Suite Bridge embedded Agent runtime started: backend="
                << backendRuntimeContext->backendId
                << std::endl;
        }

        if (backendRuntimeContext->eventStreamClient) {
            backendRuntimeContext->eventStreamClient->start();
        }
    }

    startEpgCacheWarmupWorker();
    startRecordingCacheWarmupWorker();

    std::cout << "HTTP listener runtime initialized" << std::endl;
    std::cout << "dashboard runtime initialized" << std::endl;

    std::signal(SIGINT, DaemonRuntime::handleSignal);
    std::signal(SIGTERM, DaemonRuntime::handleSignal);

    shutdownRequested_ = false;
    initialized_ = true;

    return true;
}


void DaemonRuntime::startRecordingCacheWarmupWorker()
{
    if (recordingCacheWarmupThread_.joinable()) {
        return;
    }

    recordingCacheWarmupStopRequested_.store(false);
    recordingCacheDirtyHint_.store(false);
    recordingCacheActionRefreshAttempts_.store(0);

    recordingCacheWarmupThread_ = std::thread([this]() {
        runRecordingCacheWarmupWorker();
    });
}

void DaemonRuntime::stopRecordingCacheWarmupWorker()
{
    recordingCacheWarmupStopRequested_.store(true);

    if (recordingCacheWarmupThread_.joinable()) {
        recordingCacheWarmupThread_.join();
    }
}

void DaemonRuntime::runRecordingCacheWarmupWorker()
{
    try {
        const int initialDelaySeconds = 1;
        const int dirtyDebounceSeconds = 30;
        const int metadataRefreshSeconds = 60;

        std::cout
            << "Recording cache warmup worker scheduled after "
            << initialDelaySeconds
            << " seconds"
            << std::endl;

        const auto waitForStop = [this](int seconds) -> bool {
            for (int elapsed = 0; elapsed < seconds; ++elapsed) {
                if (recordingCacheWarmupStopRequested_.load()) {
                    return true;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            return recordingCacheWarmupStopRequested_.load();
        };

        if (waitForStop(initialDelaySeconds)) {
            return;
        }

        refreshRecordingCacheForAllBackends("startup");

        auto lastRefresh = std::chrono::steady_clock::now();
        auto lastMetadataRefresh = lastRefresh;

        while (!recordingCacheWarmupStopRequested_.load()) {
            if (waitForStop(1)) {
                return;
            }

            const auto metadataNow = std::chrono::steady_clock::now();

            const auto secondsSinceMetadataRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    metadataNow - lastMetadataRefresh).count();

            if (secondsSinceMetadataRefresh >= metadataRefreshSeconds &&
                vdrRecordingCacheRepository_) {
                for (const auto& backendRuntimeContext :
                     backendRuntimeContexts_) {
                    if (recordingCacheWarmupStopRequested_.load()) {
                        return;
                    }

                    if (!backendRuntimeContext ||
                        !backendRuntimeContext
                             ->recordingMetadataEnrichmentService) {
                        continue;
                    }

                    const std::vector<VdrRecording> recordings =
                        vdrRecordingCacheRepository_->findAllForBackend(
                            backendRuntimeContext->backendId);

                    runRecordingMetadataEnrichment(
                        *backendRuntimeContext,
                        recordings,
                        recordingCacheWarmupStopRequested_,
                        "periodic");
                }

                lastMetadataRefresh = metadataNow;
            }

            int remainingActionRefreshAttempts =
                recordingCacheActionRefreshAttempts_.load();

            while (remainingActionRefreshAttempts > 0 &&
                   !recordingCacheActionRefreshAttempts_.compare_exchange_weak(
                       remainingActionRefreshAttempts,
                       remainingActionRefreshAttempts - 1)) {
            }

            if (remainingActionRefreshAttempts > 0) {
                refreshRecordingCacheForAllBackends(
                    "recording-action-reconcile");
                lastRefresh = std::chrono::steady_clock::now();
                continue;
            }

            if (!recordingCacheDirtyHint_.load()) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto secondsSinceLastRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastRefresh).count();

            if (secondsSinceLastRefresh < dirtyDebounceSeconds) {
                continue;
            }

            if (!recordingCacheDirtyHint_.exchange(false)) {
                continue;
            }

            refreshRecordingCacheForAllBackends("event-stream-dirty-hint");
            lastRefresh = std::chrono::steady_clock::now();
        }
    }
    catch (const std::exception& error) {
        std::cerr
            << "Recording cache warmup worker stopped after exception: "
            << error.what()
            << std::endl;
    }
    catch (...) {
        std::cerr
            << "Recording cache warmup worker stopped after unknown exception"
            << std::endl;
    }
}

void DaemonRuntime::refreshRecordingCacheForAllBackends(
    const std::string& reason)
{
    if (!vdrRecordingCacheRepository_) {
        std::cout
            << "Recording cache warmup skipped: repository unavailable"
            << std::endl;
        return;
    }

    if (backendRuntimeContexts_.empty()) {
        std::cout
            << "Recording cache warmup skipped: no VDR backend configured"
            << std::endl;
        return;
    }

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (recordingCacheWarmupStopRequested_.load()) {
            return;
        }

        if (!backendRuntimeContext || !backendRuntimeContext->service) {
            continue;
        }

        std::cout
            << "Recording cache warmup started: reason="
            << reason
            << ", backend="
            << backendRuntimeContext->backendId
            << std::endl;

        vdrRecordingCacheRepository_->markRefreshStarted(
            backendRuntimeContext->backendId);

        try {
            const std::vector<VdrRecording> recordings =
                backendRuntimeContext->service->getRecordings();

            const bool stored =
                vdrRecordingCacheRepository_->replaceRecordingsForBackend(
                    backendRuntimeContext->backendId,
                    recordings);

            if (stored) {
                vdrRecordingCacheRepository_->markRefreshFinished(
                    backendRuntimeContext->backendId,
                    static_cast<int>(recordings.size()));

                runRecordingMetadataEnrichment(
                    *backendRuntimeContext,
                    recordings,
                    recordingCacheWarmupStopRequested_,
                    reason);
            }
            else {
                vdrRecordingCacheRepository_->markRefreshFailed(
                    backendRuntimeContext->backendId,
                    "repository replace failed");
            }

            std::cout
                << "Recording cache warmup finished: backend="
                << backendRuntimeContext->backendId
                << ", stored="
                << (stored ? "true" : "false")
                << ", recordings="
                << recordings.size()
                << std::endl;
        }
        catch (const std::exception& error) {
            vdrRecordingCacheRepository_->markRefreshFailed(
                backendRuntimeContext->backendId,
                error.what());

            std::cerr
                << "Recording cache warmup failed: backend="
                << backendRuntimeContext->backendId
                << ", error="
                << error.what()
                << std::endl;
        }
        catch (...) {
            vdrRecordingCacheRepository_->markRefreshFailed(
                backendRuntimeContext->backendId,
                "unknown exception");

            std::cerr
                << "Recording cache warmup failed after unknown exception: backend="
                << backendRuntimeContext->backendId
                << std::endl;
        }
    }
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

void DaemonRuntime::startEpgCacheWarmupWorker()
{
    if (epgCacheWarmupThread_.joinable()) {
        return;
    }

    epgCacheWarmupStopRequested_.store(false);
    epgCacheDirtyHint_.store(false);

    epgCacheWarmupThread_ = std::thread([this]() {
        runEpgCacheWarmupWorker();
    });
}

void DaemonRuntime::stopEpgCacheWarmupWorker()
{
    epgCacheWarmupStopRequested_.store(true);

    if (epgCacheWarmupThread_.joinable()) {
        epgCacheWarmupThread_.join();
    }
}

void DaemonRuntime::runEpgCacheWarmupWorker()
{
    try {
        const int initialDelaySeconds = 20;
        const int dirtyDebounceSeconds = 120;

        std::cout
            << "EPG cache warmup worker scheduled after "
            << initialDelaySeconds
            << " seconds"
            << std::endl;

        const auto waitForStop = [this](int seconds) -> bool {
            for (int elapsed = 0; elapsed < seconds; ++elapsed) {
                if (epgCacheWarmupStopRequested_.load()) {
                    return true;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            return epgCacheWarmupStopRequested_.load();
        };

        if (waitForStop(initialDelaySeconds)) {
            return;
        }

        refreshEpgCacheForAllBackends("startup");

        auto lastRefresh = std::chrono::steady_clock::now();

        while (!epgCacheWarmupStopRequested_.load()) {
            if (waitForStop(5)) {
                return;
            }

            if (!epgCacheDirtyHint_.load()) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto secondsSinceLastRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastRefresh).count();

            if (secondsSinceLastRefresh < dirtyDebounceSeconds) {
                continue;
            }

            if (!epgCacheDirtyHint_.exchange(false)) {
                continue;
            }

            refreshEpgCacheForAllBackends("event-stream-dirty-hint");
            lastRefresh = std::chrono::steady_clock::now();
        }
    }
    catch (const std::exception& error) {
        std::cerr
            << "EPG cache warmup worker stopped after exception: "
            << error.what()
            << std::endl;
    }
    catch (...) {
        std::cerr
            << "EPG cache warmup worker stopped after unknown exception"
            << std::endl;
    }
}

void DaemonRuntime::refreshEpgCacheForAllBackends(const std::string& reason)
{
    if (backendRuntimeContexts_.empty()) {
        std::cout
            << "EPG cache warmup skipped: no VDR backend configured"
            << std::endl;
        return;
    }

    VdrEventQuery query;
    query.from = -1;
    query.timespan = 172800;
    query.channelEventLimit = 160;

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (epgCacheWarmupStopRequested_.load()) {
            return;
        }

        if (!backendRuntimeContext || !backendRuntimeContext->epgCacheService) {
            continue;
        }

        std::cout
            << "EPG cache warmup started: reason="
            << reason
            << ", backend="
            << backendRuntimeContext->backendId
            << ", timespan="
            << query.timespan
            << ", channelEventLimit="
            << query.channelEventLimit
            << std::endl;

        const EpgCacheRefreshResult result =
            backendRuntimeContext->epgCacheService->refreshBackendWindow(
                backendRuntimeContext->backendId,
                query);

        std::cout
            << "EPG cache warmup finished: backend="
            << backendRuntimeContext->backendId
            << ", accepted="
            << (result.accepted ? "true" : "false")
            << ", fetched="
            << (result.fetched ? "true" : "false")
            << ", stored="
            << (result.stored ? "true" : "false")
            << ", events="
            << result.eventCount
            << std::endl;
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

    stopRecordingCacheWarmupWorker();
    stopEpgCacheWarmupWorker();

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
