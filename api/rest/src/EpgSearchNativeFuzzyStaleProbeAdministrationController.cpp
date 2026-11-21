#include "EpgSearchNativeFuzzyStaleProbeAdministrationController.h"

#include "EpgSearchNativeFuzzyStaleProbeAdministrationService.h"

#include <sstream>
#include <vector>

namespace
{
    std::string escapeJsonString(const std::string& value)
    {
        std::ostringstream escaped;

        for (char character : value)
        {
            switch (character)
            {
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

    const char* boolJson(bool value)
    {
        return value ? "true" : "false";
    }

    std::string serializeRecords(
        const std::vector<EpgSearchNativeFuzzyStaleProbeAdministrationRecord>& records)
    {
        std::ostringstream json;
        json << "{\"staleProbes\":[";

        for (std::size_t index = 0; index < records.size(); ++index)
        {
            const auto& record = records[index];

            if (index > 0)
            {
                json << ",";
            }

            json
                << "{"
                << "\"backendId\":\"" << escapeJsonString(record.backendId) << "\","
                << "\"updatedAt\":\"" << escapeJsonString(record.updatedAt) << "\","
                << "\"ageSeconds\":" << record.ageSeconds << ","
                << "\"maxAgeSeconds\":" << record.maxAgeSeconds << ","
                << "\"status\":\"" << escapeJsonString(record.status) << "\","
                << "\"reason\":\"" << escapeJsonString(record.reason) << "\""
                << "}";
        }

        json << "]}";
        return json.str();
    }

    std::string serializeSummary(
        const EpgSearchNativeFuzzyStaleProbeAdministrationSummary& summary)
    {
        std::ostringstream json;

        json
            << "{"
            << "\"schemaReady\":" << boolJson(summary.schemaReady) << ","
            << "\"persistedResultsSeen\":" << summary.persistedResultsSeen << ","
            << "\"staleResultsFound\":" << summary.staleResultsFound << ","
            << "\"deletedResults\":" << summary.deletedResults << ","
            << "\"deleteFailures\":" << summary.deleteFailures
            << "}";

        return json.str();
    }
}

EpgSearchNativeFuzzyStaleProbeAdministrationController::
EpgSearchNativeFuzzyStaleProbeAdministrationController(
    EpgSearchNativeFuzzyStaleProbeAdministrationService& service)
    : service_(service)
{
}

ApiResponse
EpgSearchNativeFuzzyStaleProbeAdministrationController::getStaleProbeResults()
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = serializeRecords(
        service_.listStaleProbeResults());

    return response;
}

ApiResponse
EpgSearchNativeFuzzyStaleProbeAdministrationController::deleteStaleProbeResults()
{
    const auto summary =
        service_.deleteStaleProbeResults();

    ApiResponse response;
    response.statusCode = summary.deleteFailures == 0 ? 200 : 500;
    response.contentType = "application/json";
    response.body = serializeSummary(summary);

    return response;
}
