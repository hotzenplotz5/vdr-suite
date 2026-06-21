#include "ApiRouter.h"

#include "BackendRegistryController.h"
#include "CapabilityController.h"
#include "EpgController.h"
#include "JobsController.h"
#include "LiveTransportController.h"
#include "MetadataController.h"
#include "PersonController.h"
#include "Person.h"
#include "RecordingsController.h"
#include "RecordingActionExecutionController.h"
#include "RecordingActionValidationController.h"
#include "RecordingPersonSearchController.h"
#include "RestQueryParameters.h"
#include "RuntimeDiagnosticsController.h"
#include "SearchTimerController.h"
#include "SearchTimerResult.h"
#include "SnapshotChangeFeedController.h"
#include "VdrController.h"
#include "VdrRecordingQueryController.h"
#include "VdrSnapshotReadService.h"
#include "VdrTimerActionController.h"
#include "VdrTimerActionExecutorAdapterRegistry.h"

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

ApiResponse makePersonUnavailableResponse()
{
    ApiResponse response;

    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"person search unavailable\"}";

    return response;
}

ApiResponse makeRecordingPersonSearchUnavailableResponse()
{
    ApiResponse response;

    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"recording person search unavailable\"}";

    return response;
}

ApiResponse makeSearchTimerUnavailableResponse()
{
    ApiResponse response;

    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"searchtimer backend unavailable\"}";

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
    VdrSnapshotReadService& vdrSnapshotReadService,
    EpgController* epgController,
    PersonController* personController,
    RecordingPersonSearchController* recordingPersonSearchController,
    BackendRegistryController& backendRegistryController,
    CapabilityController& capabilityController,
    RecordingActionValidationController& recordingActionValidationController,
    RecordingActionExecutionController& recordingActionExecutionController,
    VdrTimerActionController& vdrTimerActionController,
    VdrTimerActionExecutorAdapterRegistry& vdrTimerActionExecutorAdapterRegistry,
    RuntimeDiagnosticsController& runtimeDiagnosticsController,
    SnapshotChangeFeedController& snapshotChangeFeedController,
    SearchTimerController* searchTimerController,
    LiveTransportController& liveTransportController)
    : dashboardController_(dashboardController),
      jobsController_(jobsController),
      recordingsController_(recordingsController),
      metadataController_(metadataController),
      vdrController_(vdrController),
      vdrRecordingQueryController_(vdrRecordingQueryController),
      vdrSnapshotReadService_(vdrSnapshotReadService),
      epgController_(epgController),
      personController_(personController),
      recordingPersonSearchController_(recordingPersonSearchController),
      backendRegistryController_(backendRegistryController),
      capabilityController_(capabilityController),
      recordingActionValidationController_(recordingActionValidationController),
      recordingActionExecutionController_(recordingActionExecutionController),
      vdrTimerActionController_(vdrTimerActionController),
      vdrTimerActionExecutorAdapterRegistry_(vdrTimerActionExecutorAdapterRegistry),
      runtimeDiagnosticsController_(runtimeDiagnosticsController),
      snapshotChangeFeedController_(snapshotChangeFeedController),
      searchTimerController_(searchTimerController),
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

    if (path == "/api/vdr/timers/actions/create")
    {
        return vdrTimerActionController_.createBody(
            body,
            vdrTimerActionExecutorAdapterRegistry_);
    }

    if (path == "/api/vdr/timers/actions/update")
    {
        return vdrTimerActionController_.updateBody(
            body,
            vdrTimerActionExecutorAdapterRegistry_);
    }

    if (path == "/api/vdr/timers/actions/delete")
    {
        return vdrTimerActionController_.removeBody(
            body,
            vdrTimerActionExecutorAdapterRegistry_);
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

    if (path == "/api/persons" ||
        path == "/api/vdr/persons")
    {
        if (personController_ == nullptr)
        {
            return makePersonUnavailableResponse();
        }

        return personController_->searchPersons(
            PersonCollection::createEmpty(),
            queryParameters.get("name"),
            queryParameters.get("normalizedName"),
            queryParameters.get("role"),
            queryParameters.get("source"),
            queryParameters.get("providerReference"),
            queryParameters.getInt("limit", 0),
            queryParameters.getInt("offset", 0));
    }

    if (path == "/api/recordings/persons/search" ||
        path == "/api/vdr/recordings/persons/search")
    {
        if (recordingPersonSearchController_ == nullptr)
        {
            return makeRecordingPersonSearchUnavailableResponse();
        }

        const std::string backendId =
            queryParameters.get("backend");

        const std::vector<VdrRecording> recordings =
            backendId.empty()
                ? vdrSnapshotReadService_.getRecordings()
                : vdrSnapshotReadService_.getRecordingsForBackend(backendId);

        return recordingPersonSearchController_->searchRecordingPersons(
            recordings,
            queryParameters.get("name"),
            queryParameters.get("normalizedName"),
            queryParameters.get("characterName"),
            queryParameters.get("role"),
            queryParameters.get("source"),
            queryParameters.get("providerReference"),
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

    if (path == "/api/epg/search")
    {
        if (epgController_ == nullptr)
        {
            return makeEpgUnavailableResponse();
        }

        return epgController_->search(
            queryParameters.get("query"),
            queryParameters.get("backend"),
            queryParameters.get("channelId"),
            queryParameters.getInt("from", -1),
            queryParameters.getInt("timespan", 7200),
            queryParameters.getInt("limit", 0),
            queryParameters.getInt("offset", 0),
            queryParameters.get("sort"),
            queryParameters.get("order"));
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

    if (path == "/api/searchtimers/preview" ||
        path == "/api/vdr/searchtimers/preview")
    {
        if (searchTimerController_ == nullptr)
        {
            return makeSearchTimerUnavailableResponse();
        }

        const std::string backendId =
            queryParameters.get("backend").empty()
                ? "default"
                : queryParameters.get("backend");

        const std::string queryText =
            queryParameters.get("query").empty()
                ? queryParameters.get("text")
                : queryParameters.get("query");

        const std::string name =
            queryParameters.get("name").empty()
                ? "Preview SearchTimer"
                : queryParameters.get("name");

        const SearchTimer previewSearchTimer =
            SearchTimer::create(
                SearchTimerId::fromBackendNativeId(
                    backendId,
                    "preview"),
                name,
                queryText,
                SearchTimerState::Active);

        const std::vector<VdrEvent> events =
            queryParameters.get("backend").empty()
                ? vdrSnapshotReadService_.getEvents()
                : vdrSnapshotReadService_.getEventsForBackend(backendId);

        return searchTimerController_->previewSearchTimer(
            previewSearchTimer,
            events,
            queryParameters.getInt("limit", 0),
            queryParameters.getInt("offset", 0));
    }

    if (path == "/api/searchtimers" ||
        path == "/api/vdr/searchtimers")
    {
        if (searchTimerController_ == nullptr)
        {
            return makeSearchTimerUnavailableResponse();
        }

        return searchTimerController_->searchSearchTimers(
            queryParameters.get("backend"),
            queryParameters.get("state"),
            queryParameters.get("text"),
            queryParameters.getInt("limit", 0),
            queryParameters.getInt("offset", 0));
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
