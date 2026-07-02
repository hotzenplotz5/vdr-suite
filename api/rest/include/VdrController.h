#pragma once

#include "DashboardController.h"

#include <string>

class VdrOverviewService;
class VdrOverviewJsonSerializer;
class VdrSnapshotReadService;
class VdrSnapshotReadJsonSerializer;
class VdrService;

class VdrController
{
public:
    VdrController(
        VdrOverviewService& overviewService,
        VdrOverviewJsonSerializer& jsonSerializer,
        VdrSnapshotReadService& snapshotReadService,
        VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer,
        VdrService* liveService = nullptr);

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
    ApiResponse getLiveTimers();
    ApiResponse getSearchTimers();
    ApiResponse getChannels();
    ApiResponse getEvents();

private:
    VdrOverviewService& overviewService_;
    VdrOverviewJsonSerializer& jsonSerializer_;
    VdrSnapshotReadService& snapshotReadService_;
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer_;
    VdrService* liveService_;
};
