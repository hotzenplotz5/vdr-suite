#include "GlobalSearchController.h"

#include "BackendRegistryService.h"
#include "GlobalSearchRepository.h"
#include "GlobalSearchResult.h"
#include "GlobalSearchService.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
constexpr int MinimumQueryLength = 2;
constexpr int DefaultLimit = 20;
constexpr int MaximumLimit = 50;
constexpr std::int64_t EpgHistorySeconds = 6 * 60 * 60;
constexpr std::int64_t DefaultEpgFutureSeconds = 14 * 24 * 60 * 60;
constexpr std::int64_t MaximumEpgWindowSeconds = 31 * 24 * 60 * 60;

std::string trimQuery(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::size_t utf8CodePointCount(const std::string& value)
{
    std::size_t count = 0;
    for (const unsigned char byte : value)
    {
        if ((byte & 0xc0U) != 0x80U) ++count;
    }
    return count;
}

std::int64_t nowEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void appendJsonString(std::ostringstream& json, const std::string& value)
{
    static const char* digits = "0123456789abcdef";
    json << '"';
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': json << "\\\""; break;
        case '\\': json << "\\\\"; break;
        case '\b': json << "\\b"; break;
        case '\f': json << "\\f"; break;
        case '\n': json << "\\n"; break;
        case '\r': json << "\\r"; break;
        case '\t': json << "\\t"; break;
        default:
            if (character < 0x20)
            {
                json << "\\u00" << digits[(character >> 4) & 0x0f]
                     << digits[character & 0x0f];
            }
            else json << static_cast<char>(character);
            break;
        }
    }
    json << '"';
}

std::string percentEncode(const std::string& value)
{
    std::ostringstream encoded;
    encoded << std::uppercase << std::hex;
    for (const unsigned char character : value)
    {
        const bool unreserved =
            (character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z') ||
            (character >= '0' && character <= '9') ||
            character == '-' || character == '_' || character == '.' || character == '~';
        if (unreserved) encoded << static_cast<char>(character);
        else encoded << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(character);
    }
    return encoded.str();
}

ApiResponse jsonResponse(int statusCode, const std::string& body)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    std::ostringstream json;
    json << "{\"error\":";
    appendJsonString(json, message);
    json << '}';
    return jsonResponse(statusCode, json.str());
}

void normalizeWindow(std::int64_t& from, std::int64_t& until)
{
    const std::int64_t now = nowEpochSeconds();
    if (from <= 0) from = now - EpgHistorySeconds;
    if (until <= from) until = now + DefaultEpgFutureSeconds;
    if (until > from + MaximumEpgWindowSeconds) until = from + MaximumEpgWindowSeconds;
}

std::string recordingArtworkUrl(const GlobalSearchRecording& item)
{
    if (item.backendNativeId.empty()) return {};
    return "/api/recordings/metadata/image?backend=" + percentEncode(item.backendId) +
        "&backendNativeId=" + percentEncode(item.backendNativeId) +
        "&kind=preferred&index=0";
}

std::string epgArtworkUrl(const GlobalSearchEpgEvent& item)
{
    if (!item.artworkAvailable) return {};
    return "/api/epg/cache/artwork?backend=" + percentEncode(item.backendId) +
        "&channelId=" + percentEncode(item.channelId) +
        "&eventId=" + percentEncode(item.eventId);
}

void appendArtwork(std::ostringstream& json, bool available, const std::string& url)
{
    json << "{\"available\":" << (available ? "true" : "false") << ",\"url\":";
    appendJsonString(json, available ? url : std::string());
    json << '}';
}

void appendRecordingMetadataArtwork(
    std::ostringstream& json,
    bool available,
    const std::string& url)
{
    const std::string exposedUrl = available ? url : std::string();
    json << "{\"presentation\":{\"posterUrl\":";
    appendJsonString(json, exposedUrl);
    json << "},\"artwork\":{\"preferredUrl\":";
    appendJsonString(json, exposedUrl);
    json << "}}";
}

std::string serialize(const GlobalSearchResult& result)
{
    std::ostringstream json;
    json << "{\"query\":";
    appendJsonString(json, result.query);
    json << ",\"backendId\":";
    appendJsonString(json, result.backendId);
    json << ",\"status\":";
    appendJsonString(json, result.status);
    json << ",\"minimumQueryLength\":" << result.minimumQueryLength
         << ",\"limit\":" << result.limit
         << ",\"offset\":" << result.offset
         << ",\"epgWindow\":{\"from\":" << result.epgFrom
         << ",\"until\":" << result.epgUntil << '}';

    json << ",\"recordings\":[";
    for (std::size_t index = 0; index < result.recordings.size(); ++index)
    {
        if (index > 0) json << ',';
        const auto& item = result.recordings[index];
        const std::string artworkUrl = recordingArtworkUrl(item);
        json << "{\"id\":"; appendJsonString(json, item.id);
        json << ",\"backendId\":"; appendJsonString(json, item.backendId);
        json << ",\"backendNativeId\":"; appendJsonString(json, item.backendNativeId);
        json << ",\"title\":"; appendJsonString(json, item.title);
        json << ",\"subtitle\":"; appendJsonString(json, item.subtitle);
        json << ",\"path\":"; appendJsonString(json, item.path);
        json << ",\"startTime\":"; appendJsonString(json, item.startTime);
        json << ",\"durationSeconds\":" << item.durationSeconds
             << ",\"sizeMb\":" << item.sizeMb
             << ",\"artwork\":";
        appendArtwork(json, item.artworkAvailable, artworkUrl);
        json << ",\"metadata\":";
        appendRecordingMetadataArtwork(json, item.artworkAvailable, artworkUrl);
        json << ",\"matchedPerson\":"; appendJsonString(json, item.matchedPerson);
        json << ",\"matchedRole\":"; appendJsonString(json, item.matchedRole);
        json << ",\"matchReason\":"; appendJsonString(json, item.matchReason);
        json << '}';
    }
    json << "],\"recordingTotal\":" << result.recordingTotal
         << ",\"recordingHasMore\":" << (result.recordingHasMore ? "true" : "false");

    json << ",\"epg\":[";
    for (std::size_t index = 0; index < result.epg.size(); ++index)
    {
        if (index > 0) json << ',';
        const auto& item = result.epg[index];
        json << "{\"id\":"; appendJsonString(json, item.eventId);
        json << ",\"eventId\":"; appendJsonString(json, item.eventId);
        json << ",\"backendId\":"; appendJsonString(json, item.backendId);
        json << ",\"channelId\":"; appendJsonString(json, item.channelId);
        json << ",\"channelName\":"; appendJsonString(json, item.channelName);
        json << ",\"title\":"; appendJsonString(json, item.title);
        json << ",\"subtitle\":"; appendJsonString(json, item.subtitle);
        json << ",\"description\":"; appendJsonString(json, item.description);
        json << ",\"startTime\":"; appendJsonString(json, item.startTime);
        json << ",\"endTime\":"; appendJsonString(json, item.endTime);
        json << ",\"durationSeconds\":" << item.durationSeconds
             << ",\"artwork\":";
        appendArtwork(json, item.artworkAvailable, epgArtworkUrl(item));
        json << ",\"matchedPerson\":"; appendJsonString(json, item.matchedPerson);
        json << ",\"matchedRole\":"; appendJsonString(json, item.matchedRole);
        json << ",\"matchReason\":"; appendJsonString(json, item.matchReason);
        json << '}';
    }
    json << "],\"epgTotal\":" << result.epgTotal
         << ",\"epgHasMore\":" << (result.epgHasMore ? "true" : "false");

    json << ",\"people\":[";
    for (std::size_t index = 0; index < result.people.size(); ++index)
    {
        if (index > 0) json << ',';
        const auto& item = result.people[index];
        json << "{\"name\":"; appendJsonString(json, item.name);
        json << ",\"role\":"; appendJsonString(json, item.role);
        json << ",\"recordingCount\":" << item.recordingCount
             << ",\"epgCount\":" << item.epgCount << '}';
    }
    json << "],\"hasMore\":"
         << ((result.recordingHasMore || result.epgHasMore) ? "true" : "false")
         << '}';
    return json.str();
}
}

GlobalSearchController::GlobalSearchController(
    GlobalSearchService& service,
    BackendRegistryService& backendRegistryService)
    : service_(service),
      backendRegistryService_(backendRegistryService)
{
}

ApiResponse GlobalSearchController::search(
    const std::string& backendId,
    const std::string& query,
    std::int64_t epgFrom,
    std::int64_t epgUntil,
    int limit,
    int offset) const
{
    const std::string normalizedBackendId =
        GlobalSearchRepository::normalizeBackendId(backendId);
    const auto backend = backendRegistryService_.getBackend(normalizedBackendId);
    if (!backend.has_value()) return jsonError(404, "backend not found");
    if (!backend->enabled) return jsonError(403, "backend is disabled");
    if (limit < 0 || offset < 0) return jsonError(400, "limit and offset must not be negative");

    const int effectiveLimit = limit == 0
        ? DefaultLimit
        : std::min(limit, MaximumLimit);
    normalizeWindow(epgFrom, epgUntil);

    const std::string normalizedQuery = trimQuery(query);
    GlobalSearchResult result;
    result.query = normalizedQuery;
    result.backendId = normalizedBackendId;
    result.limit = effectiveLimit;
    result.offset = offset;
    result.epgFrom = epgFrom;
    result.epgUntil = epgUntil;
    result.minimumQueryLength = MinimumQueryLength;

    if (normalizedQuery.empty())
    {
        result.status = "empty";
        return jsonResponse(200, serialize(result));
    }
    if (utf8CodePointCount(
            GlobalSearchRepository::foldText(normalizedQuery)) < MinimumQueryLength)
    {
        result.status = "too-short";
        return jsonResponse(200, serialize(result));
    }

    result = service_.search(
        normalizedBackendId,
        normalizedQuery,
        epgFrom,
        epgUntil,
        effectiveLimit,
        offset);
    result.minimumQueryLength = MinimumQueryLength;
    if (result.status == "unavailable")
    {
        return jsonError(503, "search index unavailable");
    }
    return jsonResponse(200, serialize(result));
}
