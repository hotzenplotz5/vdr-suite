#include "EpgCacheController.h"

#include "EpgCacheService.h"
#include "EpgCacheServiceRegistry.h"
#include "VdrEvent.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::string normalizeBackendId(const std::string& backendId)
{
    if (backendId.empty())
    {
        return "default";
    }

    return backendId;
}

std::string escapeJsonString(const std::string& value)
{
    std::ostringstream escaped;

    for (unsigned char character : value)
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
            if (character < 0x20)
            {
                escaped
                    << "\\u"
                    << std::hex
                    << std::setw(4)
                    << std::setfill('0')
                    << static_cast<int>(character)
                    << std::dec
                    << std::setfill(' ');
            }
            else
            {
                escaped << static_cast<char>(character);
            }
            break;
        }
    }

    return escaped.str();
}

const char* boolJson(bool value)
{
    return value ? "true" : "false";
}

std::string serializeEvent(const VdrEvent& event)
{
    std::ostringstream json;

    json
        << "{"
        << "\"id\":\"" << escapeJsonString(event.id) << "\","
        << "\"channelId\":\"" << escapeJsonString(event.channelId) << "\","
        << "\"title\":\"" << escapeJsonString(event.title) << "\","
        << "\"subtitle\":\"" << escapeJsonString(event.subtitle) << "\","
        << "\"description\":\"" << escapeJsonString(event.description) << "\","
        << "\"startTime\":\"" << escapeJsonString(event.startTime) << "\","
        << "\"endTime\":\"" << escapeJsonString(event.endTime) << "\","
        << "\"durationSeconds\":" << event.durationSeconds << ","
        << "\"parentalRating\":" << event.parentalRating
        << "}";

    return json.str();
}

std::string serializeEvents(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"eventCount\":" << events.size() << ","
        << "\"events\":[";

    for (std::size_t index = 0; index < events.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        json << serializeEvent(events.at(index));
    }

    json << "]}";

    return json.str();
}

std::string serializeRefreshResult(
    const std::string& backendId,
    const EpgCacheRefreshResult& result)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"accepted\":" << boolJson(result.accepted) << ","
        << "\"fetched\":" << boolJson(result.fetched) << ","
        << "\"stored\":" << boolJson(result.stored) << ","
        << "\"eventCount\":" << result.eventCount
        << "}";

    return json.str();
}

std::string serializeStatus(
    const EpgCacheStatus& status)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(status.backendId) << "\","
        << "\"ready\":" << boolJson(status.ready) << ","
        << "\"eventCount\":" << status.eventCount << ","
        << "\"lastRefreshKnown\":" << boolJson(status.lastRefreshKnown) << ","
        << "\"lastRefreshAccepted\":" << boolJson(status.lastRefreshAccepted) << ","
        << "\"lastRefreshFetched\":" << boolJson(status.lastRefreshFetched) << ","
        << "\"lastRefreshStored\":" << boolJson(status.lastRefreshStored) << ","
        << "\"lastRefreshEventCount\":" << status.lastRefreshEventCount << ","
        << "\"lastRefreshStartedAt\":" << status.lastRefreshStartedAt << ","
        << "\"lastRefreshFinishedAt\":" << status.lastRefreshFinishedAt << ","
        << "\"lastRefreshDurationMs\":" << status.lastRefreshDurationMs << ","
        << "\"lastError\":\"" << escapeJsonString(status.lastError) << "\""
        << "}";

    return json.str();
}


std::string serializeBackendNotFound(
    const std::string& backendId)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(backendId) << "\","
        << "\"status\":\"backend-not-found\","
        << "\"eventCount\":0,"
        << "\"events\":[]"
        << "}";

    return json.str();
}

ApiResponse jsonResponse(int statusCode, const std::string& body)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = body;
    return response;
}
}

EpgCacheController::EpgCacheController(EpgCacheService& service)
    : directService_(&service),
      registry_(nullptr)
{
}

EpgCacheController::EpgCacheController(EpgCacheServiceRegistry& registry)
    : directService_(nullptr),
      registry_(&registry)
{
}

EpgCacheService* EpgCacheController::findService(
    const std::string& backendId) const
{
    if (registry_ != nullptr)
    {
        return registry_->findService(backendId);
    }

    return directService_;
}

ApiResponse EpgCacheController::refreshBackendWindow(
    const std::string& backendId,
    const VdrEventQuery& query)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
    }

    const EpgCacheRefreshResult result =
        service->refreshBackendWindow(normalizedBackendId, query);

    if (!result.accepted)
    {
        return jsonResponse(
            400,
            serializeRefreshResult(normalizedBackendId, result));
    }

    if (!result.stored)
    {
        return jsonResponse(
            503,
            serializeRefreshResult(normalizedBackendId, result));
    }

    return jsonResponse(
        200,
        serializeRefreshResult(normalizedBackendId, result));
}

ApiResponse EpgCacheController::getStatus(
    const std::string& backendId) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
    }

    return jsonResponse(
        200,
        serializeStatus(service->getStatusForBackend(normalizedBackendId)));
}

ApiResponse EpgCacheController::getNowNext(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    int eventLimit) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
    }

    return jsonResponse(
        200,
        serializeEvents(
            normalizedBackendId,
            service->findNowNextForBackend(
                normalizedBackendId,
                channelId,
                fromTime,
                eventLimit)));
}

ApiResponse EpgCacheController::getWindow(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& fromTime,
    const std::string& untilTime,
    int eventLimit) const
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    EpgCacheService* service = findService(normalizedBackendId);

    if (service == nullptr)
    {
        return jsonResponse(
            404,
            serializeBackendNotFound(normalizedBackendId));
    }

    return jsonResponse(
        200,
        serializeEvents(
            normalizedBackendId,
            service->findWindowForBackend(
                normalizedBackendId,
                channelId,
                fromTime,
                untilTime,
                eventLimit)));
}
