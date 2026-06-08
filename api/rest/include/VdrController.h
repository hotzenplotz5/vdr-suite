#pragma once

#include "DashboardController.h"

class VdrOverviewService;
class VdrOverviewJsonSerializer;
class VdrSnapshotReadService;
class VdrSnapshotReadJsonSerializer;

class VdrController
{
public:
    VdrController(
        VdrOverviewService& overviewService,
        VdrOverviewJsonSerializer& jsonSerializer,
        VdrSnapshotReadService& snapshotReadService,
        VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer);

    ApiResponse getOverview();
    ApiResponse getStatus();
    ApiResponse getHealth();
    ApiResponse getSnapshotSummary();
    ApiResponse getRecordings();
    ApiResponse getTimers();
    ApiResponse getChannels();
    ApiResponse getEvents();

private:
    VdrOverviewService& overviewService_;
    VdrOverviewJsonSerializer& jsonSerializer_;
    VdrSnapshotReadService& snapshotReadService_;
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer_;
};
