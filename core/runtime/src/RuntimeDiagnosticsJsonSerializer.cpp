#include "RuntimeDiagnosticsJsonSerializer.h"

#include <sstream>

namespace {

std::string escapeJsonString(const std::string& value)
{
    std::ostringstream escaped;

    for (char character : value) {
        switch (character) {
        case '"':
            escaped << "\\\"";
            break;
        case '\\':
            escaped << "\\\\";
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
            escaped << character;
            break;
        }
    }

    return escaped.str();
}

} // namespace

std::string RuntimeDiagnosticsJsonSerializer::serialize(const RuntimeDiagnostics& diagnostics) const
{
    std::ostringstream json;

    json << "{\"measurements\":[";

    for (std::size_t index = 0; index < diagnostics.measurements.size(); ++index) {
        const RuntimeMeasurement& measurement = diagnostics.measurements[index];

        if (index > 0) {
            json << ",";
        }

        json
            << "{"
            << "\"component\":\"" << escapeJsonString(measurement.component) << "\","
            << "\"operation\":\"" << escapeJsonString(measurement.operation) << "\","
            << "\"durationMs\":" << measurement.durationMs << ","
            << "\"statusCode\":" << measurement.statusCode << ","
            << "\"sizeBytes\":" << measurement.sizeBytes
            << "}";
    }

    json << "]}";

    return json.str();
}
