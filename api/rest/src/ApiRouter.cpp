#include "ApiRouter.h"

#include "BackendRegistryController.h"
#include "CapabilityController.h"
#include "EpgController.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "RecordingsController.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RestQueryParameters.h"
#include "RuntimeDiagnosticsController.h"
#include "SnapshotChangeFeedController.h"
#include "VdrController.h"
#include "VdrRecordingQueryController.h"

#include <string>

namespace {

std::string requestPath(const std::string& requestTarget)
{
    const std::size_t queryStart = requestTarget.find('?');

    if (queryStart == std::string::npos)
    {
        return requestTarget;
    }

    return requestTarget.substr(0, queryStart);
}

std::string requestQueryString(const std::string& requestTarget)
{
    const std::size_t queryStart = requestTarget.find('?');

    if (queryStart == std::string::npos)
    {
        return "";
    }

    return requestTarget.substr(queryStart + 1);
}


ApiResponse makeEpgUnavailableResponse()
{
    ApiResponse response;

    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"epg backend unavailable\"}";

    return response;
}

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

}


ApiRouter::ApiRouter(
    DashboardController& dashboardController,
    JobsController& jobsController,
    RecordingsController& recordingsController,
    MetadataController& metadataController,
    VdrController& vdrController,
    VdrRecordingQueryController& vdrRecordingQueryController,
    EpgController* epgController,
    BackendRegistryController& backendRegistryController,
    CapabilityController& capabilityController,
    RecordingActionValidationController& recordingActionValidationController,
    RecordingActionExecutionController& recordingActionExecutionController,
    RuntimeDiagnosticsController& runtimeDiagnosticsController,
    SnapshotChangeFeedController& snapshotChangeFeedController,
    LiveTransportController& liveTransportController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController),
      metadataController_(metadataController),
      vdrController_(vdrController),
      vdrRecordingQueryController_(vdrRecordingQueryController),
      epgController_(epgController),
      backendRegistryController_(backendRegistryController),
      capabilityController_(capabilityController),
      recordingActionValidationController_(recordingActionValidationController),
      recordingActionExecutionController_(recordingActionExecutionController),
      runtimeDiagnosticsController_(runtimeDiagnosticsController),
      snapshotChangeFeedController_(snapshotChangeFeedController),
      liveTransportController_(liveTransportController)
{
}


ApiResponse ApiRouter::handlePost(
    const std::string& requestTarget,
    const std::string& body)
{
    (void)body;

    const std::string path =
        requestPath(requestTarget);

    if (path == "/api/recordings/actions/validate" ||
        path == "/api/vdr/recordings/actions/validate")
    {
        return recordingActionValidationController_.validateBody(body);
    }

    if (path == "/api/recordings/actions/execute" ||
        path == "/api/vdr/recordings/actions/execute")
    {
        return recordingActionExecutionController_.executeBody(body);
    }

    ApiResponse response;

    response.statusCode = 404;
    response.contentType = "application/json";
    response.body = "{\"error\":\"not found\"}";

    return response;
}

ApiResponse ApiRouter::handleGet(
    const std::string& requestTarget)
{
    const std::string path =
        requestPath(requestTarget);

    const RestQueryParameters queryParameters =
        RestQueryParameters::parse(
            requestQueryString(requestTarget));

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
        return capabilityController_.getCapabilities();
    }

    if (path == "/api/vdr/recordings")
    {
        return vdrController_.getRecordings();
    }

    if (path == "/api/vdr/recordings/query")
    {
        return vdrRecordingQueryController_.getRecordings(
            queryParameters.get("title"),
            queryParameters.get("backend"),
            queryParameters.get("path"),
            queryParameters.get("sort"),
            queryParameters.get("order"),
            queryParameters.get("from"),
            queryParameters.get("to"),
            queryParameters.getInt("durationMin", 0),
            queryParameters.getInt("durationMax", 0),
            queryParameters.getInt("limit", 0),
            queryParameters.getInt("offset", 0));
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

    if (path == "/api/epg/now-next")
    {
        if (epgController_ == nullptr)
        {
            return makeEpgUnavailableResponse();
        }

        return epgController_->getNowNext(
            queryParameters.get("channelId"),
            queryParameters.getInt("from", -1));
    }

    if (path == "/api/epg/time-window")
    {
        if (epgController_ == nullptr)
        {
            return makeEpgUnavailableResponse();
        }

        return epgController_->getTimeWindow(
            queryParameters.get("channelId"),
            queryParameters.getInt("from", -1),
            queryParameters.getInt("timespan", 7200));
    }

    if (path == "/api/epg/channel-window")
    {
        if (epgController_ == nullptr)
        {
            return makeEpgUnavailableResponse();
        }

        return epgController_->getChannelWindow(
            queryParameters.get("channelId"),
            queryParameters.getInt("from", -1),
            queryParameters.getInt("timespan", 7200),
            queryParameters.getInt("limit", 5));
    }

    if (path == "/api/vdr/changes")
    {
        return snapshotChangeFeedController_.getFeed();
    }

    if (path == "/api/vdr/live")
    {
        return liveTransportController_.getStream();
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
