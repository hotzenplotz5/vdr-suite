#include "RecordingActionSafetyResultJsonSerializer.h"

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

std::string RecordingActionSafetyResultJsonSerializer::serialize(
    const RecordingActionSafetyResult& result) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"canExecute\":" << boolText(result.canExecute) << ","
        << "\"dryRun\":" << boolText(result.dryRun) << ","
        << "\"readOnlyBlocked\":" << boolText(result.readOnlyBlocked) << ","
        << "\"executionDisabled\":" << boolText(result.executionDisabled) << ","
        << "\"backendUnavailable\":" << boolText(result.backendUnavailable) << ","
        << "\"recordingInUse\":" << boolText(result.recordingInUse) << ","
        << "\"missingCapability\":" << boolText(result.missingCapability) << ","
        << "\"unsupportedAction\":" << boolText(result.unsupportedAction) << ","
        << "\"blockers\":" << serializeStringArray(result.blockers) << ","
        << "\"warnings\":" << serializeStringArray(result.warnings)
        << "}";

    return json.str();
}
