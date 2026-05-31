#include "ApiRouter.h"

#include "JobsController.h"
#include "RecordingsController.h"

ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController)
{
}

ApiResponse ApiRouter::handleGet(
    const std::string& path)
{
    if (path == "/api/dashboard")
    {
        return dashboardController_.getDashboard();
    }

    if (path == "/api/jobs")
    {
        return jobsController_.getJobs();
    }

    if (path == "/api/recordings")
    {
        return recordingsController_.getRecordings();
    }

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}
