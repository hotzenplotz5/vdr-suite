#include "EpgController.h"

#include "IEpgQueryService.h"
#include "VdrEvent.h"

#include <sstream>
#include <vector>

namespace {

void appendQuoted(std::ostringstream& json, const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << ' ';
            }
            else
            {
                json << character;
            }
            break;
        }
    }

    json << '"';
}

void appendKey(std::ostringstream& json, const std::string& key)
{
    appendQuoted(json, key);
    json << ':';
}

std::string serializeEvents(const std::vector<VdrEvent>& events)
{
    std::ostringstream json;

    json << '{';
    appendKey(json, "events");
    json << '[';

    for (std::size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];

        json << '{';
        appendKey(json, "id");
        appendQuoted(json, event.id);
        json << ',';
        appendKey(json, "channelId");
        appendQuoted(json, event.channelId);
        json << ',';
        appendKey(json, "title");
        appendQuoted(json, event.title);
        json << ',';
        appendKey(json, "subtitle");
        appendQuoted(json, event.subtitle);
        json << ',';
        appendKey(json, "description");
        appendQuoted(json, event.description);
        json << ',';
        appendKey(json, "startTime");
        appendQuoted(json, event.startTime);
        json << ',';
        appendKey(json, "endTime");
        appendQuoted(json, event.endTime);
        json << ',';
        appendKey(json, "durationSeconds");
        json << event.durationSeconds;
        json << '}';

        if (i + 1 < events.size())
        {
            json << ',';
        }
    }

    json << ']';
    json << ',';
    appendKey(json, "count");
    json << events.size();
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
    return getNowNext("", -1);
}

ApiResponse EpgController::getNowNext(
    const std::string& channelId,
    int from)
{
    return makeJsonResponse(
        epgQueryService_.getNowNext(channelId, from));
}

ApiResponse EpgController::getTimeWindow()
{
    return getTimeWindow("", -1, 7200);
}

ApiResponse EpgController::getTimeWindow(
    const std::string& channelId,
    int from,
    int timespan)
{
    return makeJsonResponse(
        epgQueryService_.getTimeWindow(channelId, from, timespan));
}

ApiResponse EpgController::getChannelWindow()
{
    return getChannelWindow("", -1, 7200, 5);
}

ApiResponse EpgController::getChannelWindow(
    const std::string& channelId,
    int from,
    int timespan,
    int limit)
{
    return makeJsonResponse(
        epgQueryService_.getChannelWindow(channelId, from, timespan, limit));
}
