#include "ApiRouter.h"

#include "BackendRegistryController.h"
#include "JobsController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RuntimeDiagnosticsController.h"
#include "SnapshotChangeFeedController.h"
#include "VdrController.h"

#include <string>


static bool parseBackendSnapshotRoute(
    const std::string& path,
    std::string& backendId,
    std::string& endpoint)
{
    const std::string prefix = "/api/backends/";
    if (path.rfind(prefix, 0) != 0)
    {
        return false;
    }

    const std::string remainder =
        path.substr(prefix.size());

    const std::size_t separator =
        remainder.find('/');

    if (separator == std::string::npos)
    {
        return false;
    }

    backendId = remainder.substr(0, separator);
    endpoint = remainder.substr(separator + 1);

    return !backendId.empty() && !endpoint.empty();
}


ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController,
    MetadataController& metadataController,
    VdrController& vdrController,
    BackendRegistryController& backendRegistryController,
    RuntimeDiagnosticsController& runtimeDiagnosticsController,
    SnapshotChangeFeedController& snapshotChangeFeedController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController),
      metadataController_(metadataController),
      vdrController_(vdrController),
      backendRegistryController_(backendRegistryController),
      runtimeDiagnosticsController_(runtimeDiagnosticsController),
      snapshotChangeFeedController_(snapshotChangeFeedController)
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

    if (path == "/api/vdr/snapshots")
    {
        return vdrController_.getSnapshots();
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

    if (path == "/api/vdr/changes")
    {
        return snapshotChangeFeedController_.getFeed();
    }

    if (path == "/api/backends")
    {
        return backendRegistryController_.getBackends();
    }

    if (path == "/api/backends/default")
    {
        return backendRegistryController_.getDefaultBackend();
    }

    std::string backendId;
    std::string backendEndpoint;

    if (parseBackendSnapshotRoute(path, backendId, backendEndpoint))
    {
        if (backendEndpoint == "status")
        {
            return vdrController_.getStatusForBackend(backendId);
        }

        if (backendEndpoint == "health")
        {
            return vdrController_.getHealthForBackend(backendId);
        }

        if (backendEndpoint == "snapshot")
        {
            return vdrController_.getSnapshotSummaryForBackend(backendId);
        }
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
