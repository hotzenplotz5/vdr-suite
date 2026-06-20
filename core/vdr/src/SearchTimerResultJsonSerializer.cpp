#include "SearchTimerResultJsonSerializer.h"

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

std::string stateToJson(
    SearchTimerState state)
{
    if (state == SearchTimerState::Active) {
        return "active";
    }

    if (state == SearchTimerState::Inactive) {
        return "inactive";
    }

    return "unknown";
}

} // namespace

std::string SearchTimerResultJsonSerializer::serialize(
    const SearchTimerResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"totalCount\":" << result.totalCount() << ","
        << "\"returnedCount\":" << result.returnedCount() << ","
        << "\"limit\":" << result.limit() << ","
        << "\"offset\":" << result.offset() << ","
        << "\"searchtimers\":[";

    for (std::size_t index = 0;
         index < result.items().size();
         ++index)
    {
        const SearchTimer& timer = result.items().at(index);

        if (index > 0) {
            json << ",";
        }

        json
            << "{"
            << "\"backendId\":\"" << escapeJsonString(timer.backendId()) << "\","
            << "\"backendNativeId\":\"" << escapeJsonString(timer.backendNativeId()) << "\","
            << "\"name\":\"" << escapeJsonString(timer.name()) << "\","
            << "\"query\":\"" << escapeJsonString(timer.query()) << "\","
            << "\"state\":\"" << stateToJson(timer.state()) << "\""
            << "}";
    }

    json
        << "]"
        << "}";

    return json.str();
}