#include "EpgController.h"

#include "EpgSearchRequest.h"
#include "EpgSearchResult.h"
#include "EpgSearchResultJsonSerializer.h"
#include "EpgSearchService.h"
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

ApiResponse makeSearchJsonResponse(
    const std::string& body)
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;

    return response;
}

EpgSearchSortField parseSearchSortField(
    const std::string& value)
{
    if (value == "title")
    {
        return EpgSearchSortField::Title;
    }

    if (value == "startTime")
    {
        return EpgSearchSortField::StartTime;
    }

    if (value == "duration")
    {
        return EpgSearchSortField::Duration;
    }

    return EpgSearchSortField::None;
}

EpgSearchSortOrder parseSearchSortOrder(
    const std::string& value)
{
    if (value == "desc")
    {
        return EpgSearchSortOrder::Descending;
    }

    return EpgSearchSortOrder::Ascending;
}

}

EpgController::EpgController(
    IEpgQueryService& epgQueryService,
    EpgSearchService& epgSearchService,
    EpgSearchResultJsonSerializer& epgSearchResultJsonSerializer)
    : epgQueryService_(epgQueryService),
      epgSearchService_(epgSearchService),
      epgSearchResultJsonSerializer_(epgSearchResultJsonSerializer)
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

ApiResponse EpgController::search(
    const std::string& query,
    const std::string& backend,
    const std::string& channelId,
    int from,
    int timespan,
    int limit,
    int offset,
    const std::string& sort,
    const std::string& order)
{
    const std::vector<VdrEvent> events =
        epgQueryService_.getTimeWindow(
            channelId,
            from,
            timespan);

    const EpgSearchRequest request =
        EpgSearchRequest::sorted(
            backend,
            query,
            channelId,
            from,
            timespan,
            limit,
            offset,
            parseSearchSortField(sort),
            parseSearchSortOrder(order));

    const EpgSearchResult result =
        epgSearchService_.search(
            events,
            request);

    return makeSearchJsonResponse(
        epgSearchResultJsonSerializer_.serialize(result));
}
