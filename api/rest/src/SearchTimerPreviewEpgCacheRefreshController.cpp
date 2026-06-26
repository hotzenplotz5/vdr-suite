#include "SearchTimerPreviewEpgCacheRefreshController.h"

#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "SearchTimerPreviewEpgCacheRefreshServiceRegistry.h"

#include <sstream>
#include <string>

namespace {

std::string normalizeBackendId(const std::string& backendId)
{
    if (backendId.empty()) {
        return "default";
    }

    return backendId;
}

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

const char* boolJson(bool value)
{
    return value ? "true" : "false";
}

std::string serializeResult(
    const SearchTimerPreviewEpgCacheRefreshResult& result)
{
    std::ostringstream json;

    json
        << "{"
        << "\"backendId\":\"" << escapeJsonString(result.backendId) << "\","
        << "\"status\":\"" << escapeJsonString(result.status) << "\","
        << "\"available\":" << boolJson(result.available) << ","
        << "\"eventCount\":" << result.eventCount
        << "}";

    return json.str();
}

} // namespace

SearchTimerPreviewEpgCacheRefreshController::
SearchTimerPreviewEpgCacheRefreshController(
    SearchTimerPreviewEpgCacheRefreshServiceRegistry& registry)
    : registry_(registry)
{
}

ApiResponse SearchTimerPreviewEpgCacheRefreshController::refreshBackend(
    const std::string& backendId,
    int from,
    int timespan,
    int start,
    int limit,
    int channelEventLimit)
{
    const std::string normalizedBackendId = normalizeBackendId(backendId);

    SearchTimerPreviewEpgCacheRefreshService* service =
        registry_.findService(normalizedBackendId);

    if (service == nullptr) {
        ApiResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.body =
            "{\"backendId\":\"" + escapeJsonString(normalizedBackendId) +
            "\",\"status\":\"backend-not-found\",\"available\":false,\"eventCount\":0}";
        return response;
    }

    SearchTimerPreviewEpgCacheRefreshRequest request;
    request.backendId = normalizedBackendId;
    request.from = from;
    request.timespan = timespan;
    request.start = start;
    request.limit = limit;
    request.channelEventLimit = channelEventLimit;

    const SearchTimerPreviewEpgCacheRefreshResult result =
        service->refreshBackend(request);

    ApiResponse response;
    response.statusCode = result.available ? 200 : 503;
    response.contentType = "application/json";
    response.body = serializeResult(result);
    return response;
}
