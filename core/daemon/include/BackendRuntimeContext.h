#pragma once

#include "IHttpClient.h"
#include "IVdrAdapter.h"
#include "PollingService.h"
#include "RestfulApiSearchTimerAdapter.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
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
    std::unique_ptr<PollingService> pollingService;
};
