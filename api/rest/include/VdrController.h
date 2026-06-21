#pragma once

#include "DashboardController.h"

#include <string>

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
    ApiResponse getStatusForBackend(const std::string& backendId);
    ApiResponse getHealth();
    ApiResponse getHealthForBackend(const std::string& backendId);
    ApiResponse getSnapshotSummary();
    ApiResponse getSnapshots();
    ApiResponse getSnapshotSummaryForBackend(const std::string& backendId);
    ApiResponse getCapabilities();
    ApiResponse getRecordings();
    ApiResponse getTimers();
    ApiResponse getSearchTimers();
    ApiResponse getChannels();
    ApiResponse getEvents();

private:
    VdrOverviewService& overviewService_;
    VdrOverviewJsonSerializer& jsonSerializer_;
    VdrSnapshotReadService& snapshotReadService_;
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer_;
};
