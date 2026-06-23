#include "EpgSearchNativeFuzzyOperatorRefreshController.h"

#include "EpgSearchNativeFuzzyOperatorRefreshService.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string trim(const std::string& value)
    {
        const std::size_t first = value.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
        {
            return "";
        }

        const std::size_t last = value.find_last_not_of(" \t\n\r");
        return value.substr(first, last - first + 1);
    }

    std::string unquote(const std::string& value)
    {
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
        {
            return value.substr(1, value.size() - 2);
        }

        return value;
    }

    std::size_t findValueEnd(
        const std::string& body,
        std::size_t valueStart)
    {
        bool insideString = false;
        bool escaped = false;

        for (std::size_t index = valueStart; index < body.size(); ++index)
        {
            const char current = body[index];

            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (current == '\\' && insideString)
            {
                escaped = true;
                continue;
            }

            if (current == '"')
            {
                insideString = !insideString;
                continue;
            }

            if (!insideString && (current == ',' || current == '}'))
            {
                return index;
            }
        }

        return body.size();
    }

    std::map<std::string, std::string> parseFlatObject(
        const std::string& body)
    {
        std::map<std::string, std::string> values;
        std::size_t position = 0;

        while (position < body.size())
        {
            const std::size_t keyStart = body.find('"', position);
            if (keyStart == std::string::npos)
            {
                break;
            }

            const std::size_t keyEnd = body.find('"', keyStart + 1);
            if (keyEnd == std::string::npos)
            {
                break;
            }

            const std::size_t colon = body.find(':', keyEnd + 1);
            if (colon == std::string::npos)
            {
                break;
            }

            const std::string key =
                body.substr(keyStart + 1, keyEnd - keyStart - 1);
            const std::size_t valueEnd = findValueEnd(body, colon + 1);
            const std::string rawValue =
                trim(body.substr(colon + 1, valueEnd - colon - 1));

            if (!key.empty())
            {
                values[key] = unquote(rawValue);
            }

            if (valueEnd >= body.size() || body[valueEnd] == '}')
            {
                break;
            }

            position = valueEnd + 1;
        }

        return values;
    }

    std::string getValue(
        const std::map<std::string, std::string>& values,
        const std::string& key)
    {
        const auto iterator = values.find(key);
        return iterator == values.end() ? "" : iterator->second;
    }

    int parseInt(
        const std::map<std::string, std::string>& values,
        const std::string& key,
        int defaultValue)
    {
        const auto iterator = values.find(key);
        if (iterator == values.end() || iterator->second.empty())
        {
            return defaultValue;
        }

        return std::atoi(iterator->second.c_str());
    }

    bool parseBool(
        const std::map<std::string, std::string>& values,
        const std::string& key,
        bool defaultValue)
    {
        const auto iterator = values.find(key);
        if (iterator == values.end())
        {
            return defaultValue;
        }

        return iterator->second == "true" || iterator->second == "1";
    }

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

            json << "\"" << escapeJsonString(values[index]) << "\"";
        }

        json << "]";
        return json.str();
    }

    EpgSearchNativeFuzzyOperatorRefreshRequest parseRequest(
        const std::string& body)
    {
        EpgSearchNativeFuzzyOperatorRefreshRequest request;

        const auto values = parseFlatObject(body);

        const std::string backendId = getValue(values, "backendId").empty()
            ? getValue(values, "backend")
            : getValue(values, "backendId");
        const std::string probeQuery = getValue(values, "probeQuery").empty()
            ? getValue(values, "query")
            : getValue(values, "probeQuery");

        if (!backendId.empty())
        {
            request.backendId = backendId;
        }

        if (!probeQuery.empty())
        {
            request.probeQuery = probeQuery;
        }

        request.tolerance = parseInt(values, "tolerance", request.tolerance);
        request.keepProbeSearchTimer = parseBool(
            values,
            "keepProbeSearchTimer",
            request.keepProbeSearchTimer);
        request.updateBackendCapabilities = parseBool(
            values,
            "updateBackendCapabilities",
            request.updateBackendCapabilities);

        return request;
    }

    std::string serializeSummary(
        const EpgSearchNativeFuzzyOperatorRefreshSummary& summary)
    {
        std::ostringstream json;

        json
            << "{"
            << "\"backendId\":\"" << escapeJsonString(summary.backendId) << "\","
            << "\"backendNativeId\":\"" << escapeJsonString(summary.backendNativeId) << "\","
            << "\"probeQuery\":\"" << escapeJsonString(summary.probeQuery) << "\","
            << "\"tolerance\":" << summary.tolerance << ","
            << "\"backendKnown\":" << boolJson(summary.backendKnown) << ","
            << "\"createAttempted\":" << boolJson(summary.createAttempted) << ","
            << "\"createAccepted\":" << boolJson(summary.createAccepted) << ","
            << "\"readbackAvailable\":" << boolJson(summary.readbackAvailable) << ","
            << "\"modePreserved\":" << boolJson(summary.modePreserved) << ","
            << "\"tolerancePreserved\":" << boolJson(summary.tolerancePreserved) << ","
            << "\"cleanupAttempted\":" << boolJson(summary.cleanupAttempted) << ","
            << "\"cleanupSucceeded\":" << boolJson(summary.cleanupSucceeded) << ","
            << "\"persisted\":" << boolJson(summary.persisted) << ","
            << "\"backendCapabilitiesUpdated\":" << boolJson(summary.backendCapabilitiesUpdated) << ","
            << "\"nativeFuzzyAvailable\":" << boolJson(summary.nativeFuzzyAvailable) << ","
            << "\"status\":\"" << escapeJsonString(summary.status) << "\","
            << "\"warnings\":" << serializeStringArray(summary.warnings) << ","
            << "\"errors\":" << serializeStringArray(summary.errors)
            << "}";

        return json.str();
    }
}

EpgSearchNativeFuzzyOperatorRefreshController::
EpgSearchNativeFuzzyOperatorRefreshController(
    EpgSearchNativeFuzzyOperatorRefreshService& service)
    : service_(service)
{
}

ApiResponse EpgSearchNativeFuzzyOperatorRefreshController::refreshBody(
    const std::string& body)
{
    const auto summary =
        service_.refresh(
            parseRequest(body));

    ApiResponse response;
    response.statusCode = summary.status == "backend-not-found"
        ? 404
        : (summary.errors.empty() ? 200 : 500);
    response.contentType = "application/json";
    response.body = serializeSummary(summary);

    return response;
}
