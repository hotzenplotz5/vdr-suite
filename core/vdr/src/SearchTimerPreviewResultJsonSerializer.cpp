#include "SearchTimerPreviewResultJsonSerializer.h"

#include "EpgSearchResultJsonSerializer.h"

#include <set>
#include <sstream>

namespace
{
void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
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

const char* stateToJson(
    SearchTimerState state)
{
    switch (state)
    {
    case SearchTimerState::Active:
        return "active";
    case SearchTimerState::Inactive:
        return "inactive";
    case SearchTimerState::Unknown:
        return "unknown";
    }

    return "unknown";
}
}

std::string SearchTimerPreviewResultJsonSerializer::serialize(
    const SearchTimerPreviewResult& result) const
{
    std::ostringstream json;
    EpgSearchResultJsonSerializer epgSerializer;

    const SearchTimer& searchTimer =
        result.searchTimer();

    json << "{";
    json << "\"searchTimer\":{";
    json << "\"backendId\":";
    appendQuoted(json, searchTimer.backendId());
    json << ",\"backendNativeId\":";
    appendQuoted(json, searchTimer.backendNativeId());
    json << ",\"name\":";
    appendQuoted(json, searchTimer.name());
    json << ",\"query\":";
    appendQuoted(json, searchTimer.query());
    json << ",\"state\":";
    appendQuoted(json, stateToJson(searchTimer.state()));
    json << "},";

    std::set<std::string> channelIds;
    std::string nextStartTime;
    std::string latestStartTime;

    for (const auto& match : result.searchResult().matches())
    {
        const VdrEvent& event = match.event();

        if (!event.channelId.empty())
        {
            channelIds.insert(event.channelId);
        }

        if (!event.startTime.empty() &&
            (nextStartTime.empty() || event.startTime < nextStartTime))
        {
            nextStartTime = event.startTime;
        }

        if (!event.startTime.empty() &&
            (latestStartTime.empty() || event.startTime > latestStartTime))
        {
            latestStartTime = event.startTime;
        }
    }

    json << "\"statistics\":{";
    json << "\"totalCount\":" << result.totalCount();
    json << ",\"returnedCount\":" << result.returnedCount();
    json << ",\"channelCount\":" << channelIds.size();
    json << ",\"nextStartTime\":";
    appendQuoted(json, nextStartTime);
    json << ",\"latestStartTime\":";
    appendQuoted(json, latestStartTime);
    json << "},";

    json << "\"preview\":";
    json << epgSerializer.serialize(result.searchResult());
    json << "}";

    return json.str();
}
