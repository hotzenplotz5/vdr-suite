#pragma once

#include "DashboardController.h"

class IEpgQueryService;

class EpgController
{
public:
    explicit EpgController(
        IEpgQueryService& epgQueryService);

    ApiResponse getNowNext();
    ApiResponse getTimeWindow();
    ApiResponse getChannelWindow();

private:
    IEpgQueryService& epgQueryService_;
};
