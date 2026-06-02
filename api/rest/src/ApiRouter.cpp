#include "ApiRouter.h"

#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "VdrController.h"

ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController,
    MetadataController& metadataController,
    VdrController& vdrController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController),
      metadataController_(metadataController),
      vdrController_(vdrController)
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

    if (path == "/api/metadata")
    {
        return metadataController_.getMetadata();
    }

    if (path == "/api/vdr/overview")
    {
        return vdrController_.getOverview();
    }

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}
