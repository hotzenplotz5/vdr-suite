#include "SearchTimerCreateResultJsonSerializer.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::ostringstream escaped;

    for (const char character : value)
    {
        if (character == '\\')
        {
            escaped << "\\\\";
        }
        else if (character == '"')
        {
            escaped << "\\\"";
        }
        else
        {
            escaped << character;
        }
    }

    return escaped.str();
}

std::string serializeStringArray(
    const std::vector<std::string>& values)
{
    std::ostringstream json;

    json << "[";

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << "\"" << escapeJson(values.at(index)) << "\"";
    }

    json << "]";

    return json.str();
}

const char* boolText(bool value)
{
    return value ? "true" : "false";
}

const char* stateText(SearchTimerState state)
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

std::string SearchTimerCreateResultJsonSerializer::serialize(
    const SearchTimerCreateResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"success\":" << boolText(result.success) << ","
        << "\"backendId\":\"" << escapeJson(result.searchTimer.backendId()) << "\","
        << "\"backendNativeId\":\"" << escapeJson(result.searchTimer.backendNativeId()) << "\","
        << "\"name\":\"" << escapeJson(result.searchTimer.name()) << "\","
        << "\"query\":\"" << escapeJson(result.searchTimer.query()) << "\","
        << "\"state\":\"" << stateText(result.searchTimer.state()) << "\","
        << "\"message\":\"" << escapeJson(result.message) << "\","
        << "\"warnings\":" << serializeStringArray(result.warnings) << ","
        << "\"errors\":" << serializeStringArray(result.errors)
        << "}";

    return json.str();
}
