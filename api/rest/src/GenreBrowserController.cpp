#include "GenreBrowserController.h"

#include "BackendRegistryService.h"
#include "GenreIndexRepository.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr std::int64_t DefaultEpgWindowSeconds = 48 * 60 * 60;
constexpr std::int64_t MaximumEpgWindowSeconds = 7 * 24 * 60 * 60;

std::int64_t nowEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void appendJsonString(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';
    const char* digits = "0123456789abcdef";
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
            else
            {
                json << static_cast<char>(character);
            }
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
            character == '-' || character == '_' ||
            character == '.' || character == '~';
        if (unreserved)
        {
            encoded << static_cast<char>(character);
        }
        else
        {
            encoded << '%' << std::setw(2) << std::setfill('0')
                    << static_cast<int>(character);
        }
    }
    return encoded.str();
}

ApiResponse jsonResponse(
    int statusCode,
    const std::string& body)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(
    int statusCode,
    const std::string& message)
{
    std::ostringstream json;
    json << "{\"error\":";
    appendJsonString(json, message);
    json << "}";
    return jsonResponse(statusCode, json.str());
}

void normalizeEpgWindow(
    std::int64_t& fromTime,
    std::int64_t& untilTime)
{
    if (fromTime <= 0)
    {
        fromTime = nowEpochSeconds();
    }
    if (untilTime <= fromTime)
    {
        untilTime = fromTime + DefaultEpgWindowSeconds;
    }
    if (untilTime > fromTime + MaximumEpgWindowSeconds)
    {
        untilTime = fromTime + MaximumEpgWindowSeconds;
    }
}

void appendStringArray(
    std::ostringstream& json,
    const std::vector<std::string>& values)
{
    json << '[';
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        if (index > 0) json << ',';
        appendJsonString(json, values[index]);
    }
    json << ']';
}

void appendBrowseCategory(
    std::ostringstream& json,
    const GenreBrowseCategory& category)
{
    json << "{\"id\":";
    appendJsonString(json, category.id);
    json << ",\"label\":";
    appendJsonString(json, category.label);
    json << ",\"labelDe\":";
    appendJsonString(json, category.labelDe);
    json << ",\"labelEn\":";
    appendJsonString(json, category.labelEn);
    json << ",\"known\":true";
    json << ",\"count\":" << category.itemCount;
    json << ",\"activeCount\":" << category.itemCount;
    json << ",\"staleCount\":0";
    json << ",\"conflictCount\":0";
    json << ",\"sources\":[\"epg-browse-content-class\"]";
    json << ",\"children\":[";
    for (std::size_t index = 0;
         index < category.children.size();
         ++index)
    {
        if (index > 0) json << ',';
        appendBrowseCategory(json, category.children[index]);
    }
    json << "]}";
}

void appendBrowseCategories(
    std::ostringstream& json,
    const std::vector<GenreBrowseCategory>& categories)
{
    json << '[';
    for (std::size_t index = 0; index < categories.size(); ++index)
    {
        if (index > 0) json << ',';
        appendBrowseCategory(json, categories[index]);
    }
    json << ']';
}

int placeholderVariant(const std::string& value)
{
    std::uint32_t hash = 2166136261U;
    for (const unsigned char character : value)
    {
        hash ^= character;
        hash *= 16777619U;
    }
    return static_cast<int>(hash % 6U);
}

std::string recordingPosterUrl(const GenreRecordingItem& recording)
{
    if (recording.backendNativeId.empty()) return {};
    return "/api/recordings/metadata/image?backend=" +
        percentEncode(recording.backendId) +
        "&backendNativeId=" + percentEncode(recording.backendNativeId) +
        "&kind=preferred&index=0";
}

std::string epgArtworkUrl(const GenreEpgItem& event)
{
    if (!event.artworkAvailable) return {};
    return "/api/epg/cache/artwork?backend=" +
        percentEncode(event.backendId) +
        "&channelId=" + percentEncode(event.channelId) +
        "&eventId=" + percentEncode(event.eventId);
}

std::string epgMetadataImageUrl(
    const GenreEpgItem& event,
    const GenreEpgArtworkSelection& artwork)
{
    if (!artwork.available) return {};
    return "/api/epg/cache/metadata/image?backend=" +
        percentEncode(event.backendId) +
        "&channelId=" + percentEncode(event.channelId) +
        "&eventId=" + percentEncode(event.eventId) +
        "&kind=" + percentEncode(artwork.kind) +
        "&index=" + std::to_string(artwork.imageIndex);
}

std::string normalizedLocale(const std::string& locale)
{
    return locale.rfind("en", 0) == 0 ? "en" : "de";
}
}

GenreBrowserController::GenreBrowserController(
    GenreIndexRepository& repository,
    BackendRegistryService& backendRegistryService)
    : repository_(repository),
      backendRegistryService_(backendRegistryService)
{
}

ApiResponse GenreBrowserController::getOverview(
    const std::string& backendId,
    const std::string& scope,
    const std::string& locale,
    std::int64_t fromTime,
    std::int64_t untilTime) const
{
    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    if (!backendRegistryService_.hasBackend(normalizedBackendId))
    {
        return jsonError(404, "backend not found");
    }
    if (!repository_.ensureSchema())
    {
        return jsonError(503, "genre index unavailable");
    }

    const std::string normalizedScope =
        scope.empty() ? "recordings" : scope;
    const std::string responseLocale = normalizedLocale(locale);

    if (normalizedScope == "epg")
    {
        normalizeEpgWindow(fromTime, untilTime);
        const GenreEpgBrowseOverview overview =
            repository_.epgBrowseOverview(
                normalizedBackendId,
                fromTime,
                untilTime,
                responseLocale);

        std::ostringstream json;
        json << "{\"backendId\":";
        appendJsonString(json, overview.backendId);
        json << ",\"scope\":\"epg\"";
        json << ",\"locale\":";
        appendJsonString(json, responseLocale);
        json << ",\"taxonomy\":\"epg-browse-v1\"";
        json << ",\"from\":" << overview.fromTime;
        json << ",\"until\":" << overview.untilTime;
        json << ",\"totalItems\":" << overview.distinctItemCount;
        json << ",\"assignmentCount\":" << overview.assignmentCount;
        json << ",\"categories\":";
        appendBrowseCategories(json, overview.categories);
        json << ",\"genres\":";
        appendBrowseCategories(json, overview.categories);
        json << "}";
        return jsonResponse(200, json.str());
    }

    if (normalizedScope != "recordings")
    {
        return jsonError(400, "scope must be recordings or epg");
    }

    const GenreOverview overview = repository_.overview(
        normalizedBackendId,
        "recording",
        0,
        0,
        responseLocale);

    std::ostringstream json;
    json << "{\"backendId\":";
    appendJsonString(json, overview.backendId);
    json << ",\"scope\":\"recordings\"";
    json << ",\"locale\":";
    appendJsonString(json, responseLocale);
    json << ",\"from\":0";
    json << ",\"until\":0";
    json << ",\"totalItems\":" << overview.distinctItemCount;
    json << ",\"assignmentCount\":" << overview.assignmentCount;
    json << ",\"genres\":[";
    for (std::size_t index = 0; index < overview.genres.size(); ++index)
    {
        if (index > 0) json << ',';
        const GenreOverviewEntry& entry = overview.genres[index];
        json << "{\"id\":";
        appendJsonString(json, entry.genreId);
        json << ",\"label\":";
        appendJsonString(json, entry.label);
        json << ",\"labelDe\":";
        appendJsonString(json, entry.labelDe);
        json << ",\"labelEn\":";
        appendJsonString(json, entry.labelEn);
        json << ",\"known\":" << (entry.known ? "true" : "false");
        json << ",\"count\":" << entry.itemCount;
        json << ",\"activeCount\":" << entry.activeCount;
        json << ",\"staleCount\":" << entry.staleCount;
        json << ",\"conflictCount\":" << entry.conflictCount;
        json << ",\"sources\":";
        appendStringArray(json, entry.sources);
        json << '}';
    }
    json << "]}";
    return jsonResponse(200, json.str());
}

ApiResponse GenreBrowserController::getRecordings(
    const std::string& backendId,
    const std::string& genreId,
    int limit,
    int offset) const
{
    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    if (!backendRegistryService_.hasBackend(normalizedBackendId))
    {
        return jsonError(404, "backend not found");
    }
    if (genreId.empty())
    {
        return jsonError(400, "genre is required");
    }
    if (!repository_.ensureSchema())
    {
        return jsonError(503, "genre index unavailable");
    }
    if (!repository_.genreExists(genreId))
    {
        return jsonError(404, "genre not found");
    }

    const GenreRecordingPage page = repository_.recordingsByGenre(
        normalizedBackendId,
        genreId,
        limit,
        offset);

    std::ostringstream json;
    json << "{\"backendId\":";
    appendJsonString(json, page.backendId);
    json << ",\"genreId\":";
    appendJsonString(json, page.genreId);
    json << ",\"total\":" << page.totalCount;
    json << ",\"limit\":" << page.limit;
    json << ",\"offset\":" << page.offset;
    json << ",\"hasMore\":"
         << (page.offset + static_cast<int>(page.recordings.size()) <
                    page.totalCount
                 ? "true"
                 : "false");
    json << ",\"items\":[";
    for (std::size_t index = 0; index < page.recordings.size(); ++index)
    {
        if (index > 0) json << ',';
        const GenreRecordingItem& recording = page.recordings[index];
        const std::string posterUrl = recordingPosterUrl(recording);
        json << "{\"id\":";
        appendJsonString(json, recording.id);
        json << ",\"backendId\":";
        appendJsonString(json, recording.backendId);
        json << ",\"backendNativeId\":";
        appendJsonString(json, recording.backendNativeId);
        json << ",\"title\":";
        appendJsonString(json, recording.title);
        json << ",\"path\":";
        appendJsonString(json, recording.path);
        json << ",\"startTime\":";
        appendJsonString(json, recording.startTime);
        json << ",\"durationSeconds\":" << recording.durationSeconds;
        json << ",\"sizeMb\":" << recording.sizeMb;
        json << ",\"genreIds\":";
        appendStringArray(json, recording.genreIds);
        json << ",\"metadata\":{\"presentation\":{\"title\":";
        appendJsonString(json, recording.title);
        json << ",\"subtitle\":\"\",\"summary\":\"\",\"posterUrl\":";
        appendJsonString(json, posterUrl);
        json << ",\"placeholderVariant\":"
             << placeholderVariant(
                    recording.backendId + "\n" +
                    recording.backendNativeId);
        json << "},\"provider\":{},\"native\":{},\"artwork\":{\"preferredUrl\":";
        appendJsonString(json, posterUrl);
        json << "}}}";
    }
    json << "]}";
    return jsonResponse(200, json.str());
}

ApiResponse GenreBrowserController::getEpg(
    const std::string& backendId,
    const std::string& requestedContentClass,
    const std::string& requestedGenreId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int limit,
    int offset) const
{
    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    if (!backendRegistryService_.hasBackend(normalizedBackendId))
    {
        return jsonError(404, "backend not found");
    }
    if (!repository_.ensureSchema())
    {
        return jsonError(503, "genre index unavailable");
    }

    std::string contentClass = requestedContentClass;
    std::string genreId = requestedGenreId;
    if (contentClass.empty() &&
        GenreIndexRepository::isEpgBrowseContentClass(genreId))
    {
        contentClass = genreId;
        genreId.clear();
    }
    else if (contentClass.empty() &&
             GenreIndexRepository::isFilmGenre(genreId))
    {
        contentClass = "movie";
    }

    if (!GenreIndexRepository::isEpgBrowseContentClass(contentClass))
    {
        return jsonError(
            400,
            "contentClass must be movie, series, documentary or sports");
    }
    if (!genreId.empty() &&
        (contentClass != "movie" ||
         !GenreIndexRepository::isFilmGenre(genreId)))
    {
        return jsonError(
            400,
            "genre is only supported for canonical film genres");
    }
    if (!genreId.empty() && !repository_.genreExists(genreId))
    {
        return jsonError(404, "genre not found");
    }

    normalizeEpgWindow(fromTime, untilTime);
    const GenreEpgPage page = repository_.epgByBrowse(
        normalizedBackendId,
        contentClass,
        genreId,
        fromTime,
        untilTime,
        limit,
        offset);

    std::ostringstream json;
    json << "{\"backendId\":";
    appendJsonString(json, page.backendId);
    json << ",\"contentClass\":";
    appendJsonString(json, page.contentClass);
    json << ",\"genreId\":";
    appendJsonString(json, page.genreId);
    json << ",\"from\":" << page.fromTime;
    json << ",\"until\":" << page.untilTime;
    json << ",\"total\":" << page.totalCount;
    json << ",\"limit\":" << page.limit;
    json << ",\"offset\":" << page.offset;
    json << ",\"hasMore\":"
         << (page.offset + static_cast<int>(page.events.size()) <
                    page.totalCount
                 ? "true"
                 : "false");
    json << ",\"items\":[";
    for (std::size_t index = 0; index < page.events.size(); ++index)
    {
        if (index > 0) json << ',';
        const GenreEpgItem& event = page.events[index];
        const GenreEpgArtworkSelection poster =
            repository_.epgPortraitArtwork(
                event.backendId,
                event.channelId,
                event.eventId);
        const bool artworkAvailable = poster.available || event.artworkAvailable;
        const std::string artworkUrl = poster.available
            ? epgMetadataImageUrl(event, poster)
            : epgArtworkUrl(event);
        const int artworkWidth = poster.available
            ? poster.width
            : event.artworkWidth;
        const int artworkHeight = poster.available
            ? poster.height
            : event.artworkHeight;
        json << "{\"id\":";
        appendJsonString(json, event.eventId);
        json << ",\"eventId\":";
        appendJsonString(json, event.eventId);
        json << ",\"backendId\":";
        appendJsonString(json, event.backendId);
        json << ",\"channelId\":";
        appendJsonString(json, event.channelId);
        json << ",\"channelName\":";
        appendJsonString(json, event.channelName);
        json << ",\"title\":";
        appendJsonString(json, event.title);
        json << ",\"subtitle\":";
        appendJsonString(json, event.subtitle);
        json << ",\"description\":";
        appendJsonString(json, event.description);
        json << ",\"contentClass\":";
        appendJsonString(json, event.contentClass);
        json << ",\"startTime\":" << event.startTime;
        json << ",\"endTime\":" << event.endTime;
        json << ",\"durationSeconds\":" << event.durationSeconds;
        json << ",\"genreIds\":";
        appendStringArray(json, event.genreIds);
        json << ",\"artwork\":{\"available\":"
             << (artworkAvailable ? "true" : "false");
        json << ",\"url\":";
        appendJsonString(json, artworkUrl);
        json << ",\"width\":" << artworkWidth;
        json << ",\"height\":" << artworkHeight;
        json << "}}";
    }
    json << "]}";
    return jsonResponse(200, json.str());
}
