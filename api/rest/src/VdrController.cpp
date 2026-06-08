#include "VdrController.h"

#include "VdrOverviewJsonSerializer.h"
#include "VdrOverviewService.h"
#include "VdrSnapshotReadJsonSerializer.h"
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
