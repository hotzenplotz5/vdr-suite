#include "DaemonRuntime.h"

#include "BasicHttpClient.h"
#include "EpgEventRepository.h"
#include "GenreBrowserApiRuntime.h"
#include "GlobalSearchApiRuntime.h"
#include "LiveRemoteApiRuntime.h"
#include "RestfulApiEventStreamClient.h"
#include "RestfulApiSearchTimerAdapter.h"
#include "RestfulApiVdrAdapter.h"
#include "RestfulApiVdrTimerActionExecutorAdapter.h"

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

std::unique_ptr<BackendRuntimeContext> DaemonRuntime::createBackendRuntimeContext(
    const BackendNode& backend)
{
    VdrConfig backendConfig = backend.connection;

    if (backendRegistryService_ &&
        !GenreBrowserApiRuntime::instance().configured() &&
        !GenreBrowserApiRuntime::instance().configure(
            database_,
            *backendRegistryService_))
    {
        std::cerr << "failed to initialize genre browser metadata runtime"
                  << std::endl;
    }

    if (backendRegistryService_ &&
        !GlobalSearchApiRuntime::instance().configured() &&
        !GlobalSearchApiRuntime::instance().configure(
            database_,
            *backendRegistryService_))
    {
        std::cerr << "failed to initialize global search runtime"
                  << std::endl;
    }

    auto context = std::make_unique<BackendRuntimeContext>();

    context->backendId = backend.backendId;
    context->httpClient = std::make_unique<BasicHttpClient>(
        backendConfig.host,
        backendConfig.port,
        &runtimeLogger_,
        &runtimeDiagnosticsService_,
        [this]() {
            return shutdownRequested_.load();
        });
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

    if (backendRegistryService_ &&
        vdrSnapshotReadService_ &&
        snapshotCacheService_) {
        VdrCapabilitySet capabilities = backend.capabilities;
        capabilities.remoteControl = true;
        capabilities.liveOverlayRead = true;
        capabilities.osdView = false;
        capabilities.osdControl = false;
        backendRegistryService_->updateBackendCapabilities(
            context->backendId,
            capabilities);

        LiveRemoteApiRuntime::instance().configure(
            *backendRegistryService_,
            *vdrSnapshotReadService_,
            *snapshotCacheService_,
            [this](const std::string& changedBackendId) {
                externalVdrChangeHint_.store(true);

                if (!snapshotChangeFeed_ ||
                    !snapshotChangeFeedService_ ||
                    !liveTransportService_ ||
                    !snapshotCacheService_) {
                    return;
                }

                const int previousLatestSequenceNumber =
                    snapshotChangeFeed_->latestSequenceNumber();

                snapshotChangeFeedService_->appendChanges(
                    *snapshotChangeFeed_,
                    snapshotCacheService_->generation(),
                    {VdrChangeEvent(VdrChangeType::LiveOverlayChanged)},
                    changedBackendId);

                for (const auto& entry : snapshotChangeFeed_->entries()) {
                    if (entry.sequenceNumber() > previousLatestSequenceNumber) {
                        liveTransportService_->publishChangeFeedEntry(entry);
                    }
                }
            },
            [this](
                const std::string& backendId,
                const std::string& channelId,
                long long fromEpoch,
                int eventLimit) -> std::vector<VdrEvent> {
                if (!epgEventRepository_) {
                    return {};
                }

                return epgEventRepository_->findNowNextForBackend(
                    backendId,
                    channelId,
                    std::to_string(fromEpoch),
                    eventLimit);
            });

        LiveRemoteApiRuntime::instance().registerRestfulApiBackend(
            context->backendId,
            *context->httpClient);
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
            context->epgScraperMetadataDelegate =
                std::make_unique<SuiteBridgeEpgMetadataResolver>(
                    *context->suiteBridgeTransport);

            const RuntimeSeriesArtworkFallbackConfig& runtimeFallbackConfig =
                config_.seriesArtworkFallback();

            SeriesArtworkFallbackResolverConfig fallbackConfig;
            fallbackConfig.enabled = runtimeFallbackConfig.enabled;
            context->epgSeriesArtworkFallbackResolver =
                std::make_unique<SeriesArtworkFallbackResolver>(
                    *context->epgScraperMetadataDelegate,
                    nullptr,
                    fallbackConfig);

            ISeriesArtworkFallbackMaterializer* fallbackMaterializer = nullptr;
            if (runtimeFallbackConfig.enabled) {
                context->epgSeriesArtworkFallbackRepository =
                    std::make_unique<EpgSeriesArtworkFallbackRepository>(
                        database_);

                if (!context->epgSeriesArtworkFallbackRepository->ensureSchema()) {
                    std::cerr
                        << "failed to initialize EPG series artwork fallback schema: backend="
                        << context->backendId
                        << std::endl;
                    context->epgSeriesArtworkFallbackRepository.reset();
                }
                else {
                    FilesystemSeriesArtworkFallbackMaterializerConfig
                        materializerConfig;
                    materializerConfig.allowedSourceRoots =
                        runtimeFallbackConfig.sourceRoots;
                    materializerConfig.cacheRoot =
                        runtimeFallbackConfig.cacheRoot;
                    materializerConfig.maximumSourceBytes =
                        static_cast<std::uintmax_t>(
                            runtimeFallbackConfig.maximumSourceBytes);
                    materializerConfig.maximumDimension =
                        runtimeFallbackConfig.maximumDimension;

                    context->epgSeriesArtworkFallbackMaterializer =
                        std::make_unique<
                            FilesystemSeriesArtworkFallbackMaterializer>(
                                std::move(materializerConfig));
                    fallbackMaterializer =
                        context->epgSeriesArtworkFallbackMaterializer.get();
                }
            }

            SeriesArtworkFallbackMaterializingResolverConfig
                materializingConfig;
            materializingConfig.enabled =
                runtimeFallbackConfig.enabled &&
                fallbackMaterializer != nullptr;
            context->epgSeriesArtworkFallbackMaterializingResolver =
                std::make_unique<SeriesArtworkFallbackMaterializingResolver>(
                    *context->epgSeriesArtworkFallbackResolver,
                    fallbackMaterializer,
                    materializingConfig);

            IEpgScraperMetadataResolver* persistentDelegate =
                context->epgSeriesArtworkFallbackMaterializingResolver.get();
            if (context->epgSeriesArtworkFallbackRepository) {
                context->epgPersistentSeriesArtworkFallbackResolver =
                    std::make_unique<PersistentSeriesArtworkFallbackResolver>(
                        *persistentDelegate,
                        *context->epgSeriesArtworkFallbackRepository,
                        std::vector<std::string>{
                            runtimeFallbackConfig.cacheRoot});
                persistentDelegate =
                    context->epgPersistentSeriesArtworkFallbackResolver.get();
            }

            context->epgScraperMetadataResolver =
                std::make_unique<PersistentEpgScraperMetadataResolver>(
                    *persistentDelegate,
                    *epgArtworkRepository_);
            GenreBrowserApiRuntime::instance()
                .registerEpgScraperMetadataResolver(
                    context->backendId,
                    *context->epgScraperMetadataResolver);
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
