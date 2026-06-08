#include "ApiRouter.h"

#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "VdrController.h"

ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController,
    MetadataController& metadataController,
    VdrController& vdrController,
    RuntimeDiagnosticsController& runtimeDiagnosticsController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController),
      metadataController_(metadataController),
      vdrController_(vdrController),
      runtimeDiagnosticsController_(runtimeDiagnosticsController)
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

    if (path == "/api/vdr" ||
        path == "/api/vdr/overview")
    {
        return vdrController_.getOverview();
    }

    if (path == "/api/vdr/status")
    {
        return vdrController_.getStatus();
    }

    if (path == "/api/vdr/health")
    {
        return vdrController_.getHealth();
    }

    if (path == "/api/vdr/snapshot")
    {
        return vdrController_.getSnapshotSummary();
    }

    if (path == "/api/vdr/capabilities")
    {
        return vdrController_.getCapabilities();
    }

    if (path == "/api/vdr/recordings")
    {
        return vdrController_.getRecordings();
    }

    if (path == "/api/vdr/timers")
    {
        return vdrController_.getTimers();
    }

    if (path == "/api/vdr/channels")
    {
        return vdrController_.getChannels();
    }

    if (path == "/api/vdr/events")
    {
        return vdrController_.getEvents();
    }

    if (path == "/api/runtime/summary" ||
        path == "/api/runtime/diagnostics/summary")
    {
        return runtimeDiagnosticsController_.getRuntimeDiagnosticsSummary();
    }

    if (path == "/api/runtime" ||
        path == "/api/runtime/diagnostics")
    {
        return runtimeDiagnosticsController_.getRuntimeDiagnostics();
    }

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}
