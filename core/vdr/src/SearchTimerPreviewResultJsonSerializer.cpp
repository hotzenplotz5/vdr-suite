#include "SearchTimerPreviewResultJsonSerializer.h"

#include "EpgSearchResultJsonSerializer.h"

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

    json << "\"preview\":";
    json << epgSerializer.serialize(result.searchResult());
    json << "}";

    return json.str();
}
