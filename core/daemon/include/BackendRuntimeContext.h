#pragma once

#include "EpgArtworkEnrichmentService.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheService.h"
#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "PollingService.h"
#include "RestfulApiEventStreamClient.h"
#include "RestfulApiSearchTimerAdapter.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "SuiteBridgeEmbeddedAgentRuntime.h"
#include "SuiteBridgeEpgArtworkResolver.h"
#include "SuiteBridgeSvdrpTransport.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

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
    std::unique_ptr<EpgArtworkRepository> epgArtworkRepository;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeSvdrpTransport> epgArtworkTransport;
    std::unique_ptr<SuiteBridgeEpgArtworkResolver> epgArtworkResolver;
    std::unique_ptr<EpgArtworkEnrichmentService> epgArtworkEnrichmentService;
    std::unique_ptr<EpgCacheService> epgCacheService;
    std::unique_ptr<PollingService> pollingService;
    std::unique_ptr<RestfulApiEventStreamClient> eventStreamClient;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeEmbeddedAgentRuntime> suiteBridgeAgentRuntime;
};