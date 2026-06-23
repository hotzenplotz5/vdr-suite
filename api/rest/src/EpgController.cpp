#include "EpgController.h"

#include "EpgSearchRequest.h"
#include "EpgSearchRequestMapper.h"
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

ApiResponse makeBadRequestResponse(
    const std::string& message)
{
    ApiResponse response;

    response.statusCode = 400;
    response.contentType = "application/json";
    response.body = "{\"error\":\"" + message + "\"}";

    return response;
}

bool validSearchSort(
    const std::string& value)
{
    return value.empty() ||
           value == "title" ||
           value == "startTime" ||
           value == "duration";
}

bool validSearchOrder(
    const std::string& value)
{
    return value.empty() ||
           value == "asc" ||
           value == "desc";
}

bool validSearchMode(
    const std::string& value)
{
    return value.empty() ||
           value == "phrase" ||
           value == "exact" ||
           value == "all" ||
           value == "allWords" ||
           value == "any" ||
           value == "anyWord";
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

EpgSearchMode parseSearchMode(
    const std::string& value)
{
    if (value == "exact")
    {
        return EpgSearchMode::Exact;
    }

    if (value == "all" || value == "allWords")
    {
        return EpgSearchMode::AllWords;
    }

    if (value == "any" || value == "anyWord")
    {
        return EpgSearchMode::AnyWord;
    }

    return EpgSearchMode::Phrase;
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
    return search(
        query,
        backend,
        channelId,
        from,
        timespan,
        limit,
        offset,
        sort,
        order,
        "");
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
    const std::string& order,
    const std::string& mode)
{
    if (timespan <= 0)
    {
        return makeBadRequestResponse("timespan must be greater than zero");
    }

    if (limit < 0)
    {
        return makeBadRequestResponse("limit must not be negative");
    }

    if (offset < 0)
    {
        return makeBadRequestResponse("offset must not be negative");
    }

    if (!validSearchSort(sort))
    {
        return makeBadRequestResponse("invalid sort field");
    }

    if (!validSearchOrder(order))
    {
        return makeBadRequestResponse("invalid sort order");
    }

    if (!validSearchMode(mode))
    {
        return makeBadRequestResponse("invalid search mode");
    }

    const std::vector<VdrEvent> events =
        epgQueryService_.getTimeWindow(
            channelId,
            from,
            timespan);

    EpgSearchRequest request =
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

    if (!mode.empty())
    {
        request.setSearchMode(parseSearchMode(mode));
    }

    const EpgSearchRequestMapper requestMapper;

    const EpgSearchResult result =
        epgSearchService_.search(
            events,
            requestMapper.map(request));

    return makeSearchJsonResponse(
        epgSearchResultJsonSerializer_.serialize(result));
}
