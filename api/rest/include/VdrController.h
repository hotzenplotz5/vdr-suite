#pragma once

#include "DashboardController.h"

class VdrOverviewService;
class VdrOverviewJsonSerializer;

class VdrController
{
public:
    VdrController(
        VdrOverviewService& overviewService,
        VdrOverviewJsonSerializer& jsonSerializer);

    ApiResponse getOverview();

private:
    VdrOverviewService& overviewService_;
    VdrOverviewJsonSerializer& jsonSerializer_;
};
