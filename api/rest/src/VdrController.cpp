#include "VdrController.h"

#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrSnapshotReadJsonSerializer.h"
#include "VdrCapabilitySet.h"
#include "CapabilityResolver.h"
#include "VdrSnapshotReadService.h"

VdrController::VdrController(
    VdrOverviewService& overviewService,
    VdrOverviewJsonSerializer& jsonSerializer,
    VdrSnapshotReadService& snapshotReadService,
    VdrSnapshotReadJsonSerializer& snapshotReadJsonSerializer)
    : overviewService_(overviewService),
      jsonSerializer_(jsonSerializer),
      snapshotReadService_(snapshotReadService),
      snapshotReadJsonSerializer_(snapshotReadJsonSerializer)
{
}

ApiResponse VdrController::getOverview()
{
    ApiResponse response;

    const auto overview =
        overviewService_.getOverview();

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        jsonSerializer_.serialize(overview);

    return response;
}

ApiResponse VdrController::getStatus()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeStatus(
            snapshotReadService_.getStatus());

    return response;
}

ApiResponse VdrController::getStatusForBackend(
    const std::string& backendId)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeStatus(
            snapshotReadService_.getStatusForBackend(backendId));

    return response;
}

ApiResponse VdrController::getHealth()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeHealth(
            snapshotReadService_.hasSnapshot(),
            snapshotReadService_.getStatus(),
            snapshotReadService_.getChannels().size(),
            snapshotReadService_.getEvents().size(),
            snapshotReadService_.getTimers().size(),
            snapshotReadService_.getRecordings().size());

    return response;
}

ApiResponse VdrController::getHealthForBackend(
    const std::string& backendId)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeHealth(
            snapshotReadService_.hasSnapshotForBackend(backendId),
            snapshotReadService_.getStatusForBackend(backendId),
            snapshotReadService_.getChannelsForBackend(backendId).size(),
            snapshotReadService_.getEventsForBackend(backendId).size(),
            snapshotReadService_.getTimersForBackend(backendId).size(),
            snapshotReadService_.getRecordingsForBackend(backendId).size(),
            backendId);

    return response;
}

ApiResponse VdrController::getCapabilities()
{
    ApiResponse response;

    const VdrCapabilitySet capabilities =
        VdrCapabilitySet::snapshotReadOnly();

    const CapabilityResolver resolver(capabilities);

    VdrCapabilitySet resolvedCapabilities;
    resolvedCapabilities.snapshotRead =
        resolver.supports("snapshot.read");
    resolvedCapabilities.statusRead =
        resolver.supports("status.read");
    resolvedCapabilities.healthRead =
        resolver.supports("health.read");
    resolvedCapabilities.recordingsRead =
        resolver.supports("recordings.read");
    resolvedCapabilities.timersRead =
        resolver.supports("timers.read");
    resolvedCapabilities.channelsRead =
        resolver.supports("channels.read");
    resolvedCapabilities.eventsRead =
        resolver.supports("events.read");

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeCapabilities(
            resolvedCapabilities);

    return response;
}


ApiResponse VdrController::getSnapshots()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeSnapshots(
            snapshotReadService_.getSnapshots());

    return response;
}

ApiResponse VdrController::getSnapshotSummary()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeSnapshotSummary(
            snapshotReadService_.hasSnapshot(),
            snapshotReadService_.getChannels().size(),
            snapshotReadService_.getEvents().size(),
            snapshotReadService_.getTimers().size(),
            snapshotReadService_.getRecordings().size());

    return response;
}

ApiResponse VdrController::getSnapshotSummaryForBackend(
    const std::string& backendId)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeSnapshotSummary(
            snapshotReadService_.hasSnapshotForBackend(backendId),
            snapshotReadService_.getChannelsForBackend(backendId).size(),
            snapshotReadService_.getEventsForBackend(backendId).size(),
            snapshotReadService_.getTimersForBackend(backendId).size(),
            snapshotReadService_.getRecordingsForBackend(backendId).size(),
            backendId);

    return response;
}

ApiResponse VdrController::getRecordings()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeRecordings(
            snapshotReadService_.getRecordings());

    return response;
}

ApiResponse VdrController::getTimers()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeTimers(
            snapshotReadService_.getTimers());

    return response;
}

ApiResponse VdrController::getSearchTimers()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeSearchTimers(
            snapshotReadService_.getSearchTimers());

    return response;
}

ApiResponse VdrController::getChannels()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeChannels(
            snapshotReadService_.getChannels());

    return response;
}

ApiResponse VdrController::getEvents()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body =
        snapshotReadJsonSerializer_.serializeEvents(
            snapshotReadService_.getEvents());

    return response;
}
