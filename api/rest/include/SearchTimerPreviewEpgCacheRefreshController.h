#pragma once

#include "DashboardController.h"

#include <string>

class SearchTimerPreviewEpgCacheRefreshServiceRegistry;

class SearchTimerPreviewEpgCacheRefreshController
{
public:
    explicit SearchTimerPreviewEpgCacheRefreshController(
        SearchTimerPreviewEpgCacheRefreshServiceRegistry& registry);

    ApiResponse refreshBackend(
        const std::string& backendId,
        int from,
        int timespan,
        int start,
        int limit,
        int channelEventLimit);

private:
    SearchTimerPreviewEpgCacheRefreshServiceRegistry& registry_;
};
