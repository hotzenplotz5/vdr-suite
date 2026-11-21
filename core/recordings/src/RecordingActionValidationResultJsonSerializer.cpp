#include "RecordingActionValidationResultJsonSerializer.h"

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

std::string RecordingActionValidationResultJsonSerializer::serialize(
    const RecordingActionValidationResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"valid\":" << boolText(result.valid) << ","
        << "\"dryRun\":" << boolText(result.dryRun) << ","
        << "\"wouldCreateJob\":" << boolText(result.wouldCreateJob) << ","
        << "\"backendId\":\"" << escapeJson(result.backendId) << "\","
        << "\"recordingId\":\"" << escapeJson(result.recordingId) << "\","
        << "\"requiredCapabilities\":"
        << serializeStringArray(result.requiredCapabilities) << ","
        << "\"requiredPermissions\":"
        << serializeStringArray(result.requiredPermissions) << ","
        << "\"warnings\":"
        << serializeStringArray(result.warnings) << ","
        << "\"errors\":"
        << serializeStringArray(result.errors)
        << "}";

    return json.str();
}
