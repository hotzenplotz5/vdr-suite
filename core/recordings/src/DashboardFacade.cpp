#include "DashboardFacade.h"

#include "JobDashboardService.h"
#include "RecordingDashboardService.h"

DashboardFacade::DashboardFacade(
    JobDashboardService& jobDashboardService,
    RecordingDashboardService& recordingDashboardService)
    : jobDashboardService_(jobDashboardService),
      recordingDashboardService_(recordingDashboardService)
{
}

DashboardOverview DashboardFacade::getOverview()
{
    DashboardOverview overview;

    overview.jobs =
        jobDashboardService_.getSummary();

    overview.recordings =
        recordingDashboardService_.getSummary();

    return overview;
}
