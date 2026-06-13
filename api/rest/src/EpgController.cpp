#include "EpgController.h"

#include "IEpgQueryService.h"
#include "VdrEvent.h"

#include <sstream>
#include <vector>

namespace {

void appendQuoted(std::ostringstream& json, const std::string& value)
{
    json << '"' << value << '"';
}

std::string serializeEvents(const std::vector<VdrEvent>& events)
{
    std::ostringstream json;

    json << '{';
    json << '"' << "events" << '"' << ':' << '[';

    for (std::size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];

        json << '{';
        json << '"' << "id" << '"' << ':';
        appendQuoted(json, event.id);
        json << ',';
        json << '"' << "channelId" << '"' << ':';
        appendQuoted(json, event.channelId);
        json << ',';
        json << '"' << "title" << '"' << ':';
        appendQuoted(json, event.title);
        json << ',';
        json << '"' << "subtitle" << '"' << ':';
        appendQuoted(json, event.subtitle);
        json << ',';
        json << '"' << "description" << '"' << ':';
        appendQuoted(json, event.description);
        json << ',';
        json << '"' << "startTime" << '"' << ':';
        appendQuoted(json, event.startTime);
        json << ',';
        json << '"' << "endTime" << '"' << ':';
        appendQuoted(json, event.endTime);
        json << ',';
        json << '"' << "durationSeconds" << '"' << ':';
        json << event.durationSeconds;
        json << '}';

        if (i + 1 < events.size())
        {
            json << ',';
        }
    }

    json << ']';
    json << ',';
    json << '"' << "count" << '"' << ':' << events.size();
    json << '}';

    return json.str();
}

ApiResponse makeJsonResponse(const std::vector<VdrEvent>& events)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializeEvents(events);

    return response;
}

}

EpgController::EpgController(
    IEpgQueryService& epgQueryService)
    : epgQueryService_(epgQueryService)
{
}

ApiResponse EpgController::getNowNext()
{
    return makeJsonResponse(
        epgQueryService_.getNowNext("", -1));
}

ApiResponse EpgController::getTimeWindow()
{
    return makeJsonResponse(
        epgQueryService_.getTimeWindow("", -1, 7200));
}

ApiResponse EpgController::getChannelWindow()
{
    return makeJsonResponse(
        epgQueryService_.getChannelWindow("", -1, 7200, 5));
}
