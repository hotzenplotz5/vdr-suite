#pragma once

#include "DashboardController.h"

#include <string>

class EpgSearchResultJsonSerializer;
class EpgSearchService;
class IEpgQueryService;

class EpgController
{
public:
    EpgController(
        IEpgQueryService& epgQueryService,
        EpgSearchService& epgSearchService,
        EpgSearchResultJsonSerializer& epgSearchResultJsonSerializer);

    ApiResponse getNowNext();
    ApiResponse getNowNext(
        const std::string& channelId,
        int from);

    ApiResponse getTimeWindow();
    ApiResponse getTimeWindow(
        const std::string& channelId,
        int from,
        int timespan);

    ApiResponse getChannelWindow();
    ApiResponse getChannelWindow(
        const std::string& channelId,
        int from,
        int timespan,
        int limit);

    ApiResponse search(
        const std::string& query,
        const std::string& backend,
        const std::string& channelId,
        int from,
        int timespan,
        int limit,
        int offset,
        const std::string& sort,
        const std::string& order);

private:
    IEpgQueryService& epgQueryService_;
    EpgSearchService& epgSearchService_;
    EpgSearchResultJsonSerializer& epgSearchResultJsonSerializer_;
};
