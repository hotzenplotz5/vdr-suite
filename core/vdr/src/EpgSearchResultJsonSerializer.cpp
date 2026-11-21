#include "EpgSearchResultJsonSerializer.h"

#include "EpgSearchMatch.h"
#include "VdrEvent.h"

#include <sstream>
#include <string>

namespace {
std::string escapeJsonString(
    const std::string& value)
{
    std::ostringstream escaped;

    for (char c : value) {
        switch (c) {
        case '\\':
            escaped << "\\\\";
            break;
        case '"':
            escaped << "\\\"";
            break;
        case '\n':
            escaped << "\\n";
            break;
        case '\r':
            escaped << "\\r";
            break;
        case '\t':
            escaped << "\\t";
            break;
        default:
            escaped << c;
            break;
        }
    }

    return escaped.str();
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << "[";

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << "\"" << escapeJsonString(values.at(index)) << "\"";
    }

    json << "]";
}
}

std::string EpgSearchResultJsonSerializer::serialize(
    const EpgSearchResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"totalCount\":" << result.totalCount() << ","
        << "\"returnedCount\":" << result.returnedCount() << ","
        << "\"limit\":" << result.limit() << ","
        << "\"offset\":" << result.offset() << ","
        << "\"matches\":[";

    for (std::size_t index = 0;
         index < result.matches().size();
         ++index)
    {
        const EpgSearchMatch& match =
            result.matches().at(index);
        const VdrEvent& event =
            match.event();

        if (index > 0)
        {
            json << ",";
        }

        json
            << "{"
            << "\"backendId\":\"" << escapeJsonString(match.backendId()) << "\","
            << "\"matchedFields\":";
        appendStringArray(json, match.matchedFields());

        json
            << ","
            << "\"event\":{"
            << "\"id\":\"" << escapeJsonString(event.id) << "\","
            << "\"channelId\":\"" << escapeJsonString(event.channelId) << "\","
            << "\"title\":\"" << escapeJsonString(event.title) << "\","
            << "\"subtitle\":\"" << escapeJsonString(event.subtitle) << "\","
            << "\"description\":\"" << escapeJsonString(event.description) << "\","
            << "\"startTime\":\"" << escapeJsonString(event.startTime) << "\","
            << "\"endTime\":\"" << escapeJsonString(event.endTime) << "\","
            << "\"durationSeconds\":" << event.durationSeconds << ","
            << "\"contentDescriptors\":";
        appendStringArray(json, event.contentDescriptors);

        json
            << ","
            << "\"parentalRating\":" << event.parentalRating
            << "}"
            << "}";
    }

    json
        << "]"
        << "}";

    return json.str();
}
