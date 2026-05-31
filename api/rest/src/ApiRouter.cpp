#include "ApiRouter.h"

#include "JobsController.h"

ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController)
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

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}
