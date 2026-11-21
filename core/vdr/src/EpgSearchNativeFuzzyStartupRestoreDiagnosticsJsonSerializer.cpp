#include "EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer.h"

#include <sstream>

namespace
{
    const char* boolJson(bool value)
    {
        return value ? "true" : "false";
    }
}

std::string EpgSearchNativeFuzzyStartupRestoreDiagnosticsJsonSerializer::serialize(
    const EpgSearchNativeFuzzyStartupRestoreDiagnostics& diagnostics) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"schemaReady\":" << boolJson(diagnostics.schemaReady) << ","
        << "\"backendsSeen\":" << diagnostics.backendsSeen << ","
        << "\"persistedResultsFound\":" << diagnostics.persistedResultsFound << ","
        << "\"backendsUpdated\":" << diagnostics.backendsUpdated << ","
        << "\"nativeFuzzyAvailable\":" << diagnostics.nativeFuzzyAvailable << ","
        << "\"nativeFuzzyUnavailable\":" << diagnostics.nativeFuzzyUnavailable << ","
        << "\"staleResultsIgnored\":" << diagnostics.staleResultsIgnored << ","
        << "\"status\":\"" << diagnostics.status() << "\","
        << "\"reason\":\"" << diagnostics.reason() << "\""
        << "}";

    return json.str();
}
