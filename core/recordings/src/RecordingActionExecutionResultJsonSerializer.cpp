#include "RecordingActionExecutionResultJsonSerializer.h"

#include "RecordingActionUtils.h"

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

const char* boolText(
    bool value)
{
    return value ? "true" : "false";
}
}

std::string RecordingActionExecutionResultJsonSerializer::serialize(
    const RecordingActionExecutionResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"success\":" << boolText(result.success) << ","
        << "\"type\":\"" << escapeJson(toString(result.type)) << "\","
        << "\"backendId\":\"" << escapeJson(result.backendId) << "\","
        << "\"recordingId\":\"" << escapeJson(result.recordingId) << "\","
        << "\"backendNativeId\":\"" << escapeJson(result.backendNativeId) << "\","
        << "\"recordingPath\":\"" << escapeJson(result.recordingPath) << "\","
        << "\"snapshotRefreshed\":" << boolText(result.snapshotRefreshed) << ","
        << "\"upstreamHttpStatus\":" << result.upstreamHttpStatus << ","
        << "\"upstreamEndpoint\":\"" << escapeJson(result.upstreamEndpoint) << "\","
        << "\"upstreamResponseBody\":\"" << escapeJson(result.upstreamResponseBody) << "\","
        << "\"message\":\"" << escapeJson(result.message) << "\","
        << "\"warnings\":" << serializeStringArray(result.warnings) << ","
        << "\"errors\":" << serializeStringArray(result.errors)
        << "}";

    return json.str();
}
