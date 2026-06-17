#include "RecordingActionRequestPreviewResultJsonSerializer.h"

#include "RecordingActionUtils.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string escapeJson(
    const std::string& value)
{
    std::ostringstream escaped;

    for (char character : value)
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

std::string serializeStringMap(
    const std::map<std::string, std::string>& values)
{
    std::ostringstream json;

    json << "{";

    std::size_t index = 0;

    for (const auto& value : values)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << "\"" << escapeJson(value.first) << "\":"
             << "\"" << escapeJson(value.second) << "\"";

        ++index;
    }

    json << "}";

    return json.str();
}

const char* boolText(
    bool value)
{
    return value ? "true" : "false";
}
}

std::string RecordingActionRequestPreviewResultJsonSerializer::serialize(
    const RecordingActionRequestPreviewResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"success\":" << boolText(result.success) << ","
        << "\"type\":\"" << escapeJson(toString(result.type)) << "\","
        << "\"backendId\":\"" << escapeJson(result.backendId) << "\","
        << "\"recordingId\":\"" << escapeJson(result.recordingId) << "\","
        << "\"method\":\"" << escapeJson(result.method) << "\","
        << "\"url\":\"" << escapeJson(result.url) << "\","
        << "\"headers\":" << serializeStringMap(result.headers) << ","
        << "\"body\":\"" << escapeJson(result.body) << "\","
        << "\"message\":\"" << escapeJson(result.message) << "\","
        << "\"warnings\":" << serializeStringArray(result.warnings) << ","
        << "\"errors\":" << serializeStringArray(result.errors)
        << "}";

    return json.str();
}
