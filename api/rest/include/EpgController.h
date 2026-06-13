#pragma once

#include "DashboardController.h"

#include <string>

class IEpgQueryService;

class EpgController
{
public:
    explicit EpgController(
        IEpgQueryService& epgQueryService);

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

private:
    IEpgQueryService& epgQueryService_;
};
