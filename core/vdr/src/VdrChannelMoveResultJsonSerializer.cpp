
#include "VdrChannelMoveResultJsonSerializer.h"

#include <sstream>

namespace
{
std::string escapeJson(
    const std::string& value)
{
    std::string escaped;

    for (const char character : value)
    {
        switch (character)
        {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += character;
            break;
        }
    }

    return escaped;
}

void appendStringArray(
    std::ostringstream& stream,
    const std::vector<std::string>& values)
{
    stream << "[";

    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0)
        {
            stream << ",";
        }

        stream << "\"" << escapeJson(values[index]) << "\"";
    }

    stream << "]";
}
}

std::string VdrChannelMoveResultJsonSerializer::serialize(
    const VdrChannelMoveResult& result) const
{
    std::ostringstream stream;

    stream << "{";
    stream << "\"success\":" << (result.success ? "true" : "false");
    stream << ",\"dryRun\":" << (result.dryRun ? "true" : "false");
    stream << ",\"backendId\":\"" << escapeJson(result.backendId) << "\"";
    stream << ",\"sourceNumber\":" << result.sourceNumber;
    stream << ",\"targetNumber\":" << result.targetNumber;
    stream << ",\"message\":\"" << escapeJson(result.message) << "\"";
    stream << ",\"command\":\"" << escapeJson(result.command) << "\"";
    stream << ",\"rawOutput\":\"" << escapeJson(result.rawOutput) << "\"";
    stream << ",\"errors\":";
    appendStringArray(stream, result.errors);
    stream << "}";

    return stream.str();
}
