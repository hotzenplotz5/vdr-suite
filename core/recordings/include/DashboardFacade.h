#pragma once

#include "DashboardOverview.h"

class JobDashboardService;
class RecordingDashboardService;

class DashboardFacade
{
public:
    DashboardFacade(
        JobDashboardService& jobDashboardService,
        RecordingDashboardService& recordingDashboardService);

    DashboardOverview getOverview();

private:
    JobDashboardService& jobDashboardService_;
    RecordingDashboardService& recordingDashboardService_;
};
