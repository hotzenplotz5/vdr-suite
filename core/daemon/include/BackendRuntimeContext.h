#pragma once

#include "EpgArtworkEnrichmentService.h"
#include "EpgCacheService.h"
#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "PersistentEpgScraperMetadataResolver.h"
#include "PollingService.h"
#include "RestfulApiEventStreamClient.h"
#include "RestfulApiSearchTimerAdapter.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "SeriesArtworkFallbackResolver.h"
#include "SuiteBridgeEmbeddedAgentRuntime.h"
#include "SuiteBridgeEpgArtworkResolver.h"
#include "SuiteBridgeEpgMetadataResolver.h"
#include "SuiteBridgeRecordingMetadataResolver.h"
#include "SuiteBridgeSvdrpTransport.h"
#include "VdrRecordingNativeMetadataEnrichmentService.h"
#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cstdint>
#include <memory>
#include <string>

struct BackendRuntimeContext
{
    std::string backendId;

    std::unique_ptr<IHttpClient> httpClient;
    std::unique_ptr<IVdrAdapter> adapter;
    std::unique_ptr<VdrService> service;
    std::unique_ptr<RestfulApiSearchTimerAdapter> searchTimerAdapter;
    std::unique_ptr<VdrSnapshotBuilder> snapshotBuilder;
    std::unique_ptr<SearchTimerPreviewEpgCacheRefreshService> searchTimerPreviewEpgCacheRefreshService;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeSvdrpTransport> suiteBridgeTransport;
    std::unique_ptr<SuiteBridgeEpgArtworkResolver> epgArtworkResolver;
    std::unique_ptr<SuiteBridgeEpgMetadataResolver> epgScraperMetadataDelegate;
    std::unique_ptr<SeriesArtworkFallbackResolver> epgSeriesArtworkFallbackResolver;
    std::unique_ptr<PersistentEpgScraperMetadataResolver> epgScraperMetadataResolver;
    std::unique_ptr<EpgArtworkEnrichmentService> epgArtworkEnrichmentService;
    std::unique_ptr<VdrRecordingNativeMetadataRepository> recordingMetadataRepository;
    std::unique_ptr<SuiteBridgeRecordingMetadataResolver> recordingMetadataResolver;
    std::unique_ptr<VdrRecordingNativeMetadataEnrichmentService> recordingMetadataEnrichmentService;
    std::unique_ptr<EpgCacheService> epgCacheService;
    std::unique_ptr<PollingService> pollingService;
    std::unique_ptr<RestfulApiEventStreamClient> eventStreamClient;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime;

    std::int64_t epgTypeSnapshotFrom = 0;
    std::int64_t epgTypeSnapshotUntil = 0;
    std::uint64_t epgTypeSnapshotOffset = 0;
    bool epgTypeSnapshotComplete = true;
    bool epgTypeSnapshotSupported = true;
};
