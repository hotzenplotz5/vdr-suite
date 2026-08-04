#include "VdrRecordingFolderController.h"

#include "EpgArtworkController.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingMetadataJsonSerializer.h"
#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>

namespace
{

void appendJsonString(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
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
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << "\\u00";
                const char* digits = "0123456789abcdef";
                json << digits[(character >> 4) & 0x0f];
                json << digits[character & 0x0f];
            }
            else json << character;
            break;
        }
    }

    json << '"';
}

void appendRecordingJson(
    std::ostringstream& json,
    const VdrRecording& recording)
{
    json << "{";
    json << "\"id\":";
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
    json << ",\"metadata\":"
         << VdrRecordingMetadataJsonSerializer::serialize(recording);
    json << "}";
}

ApiResponse jsonResponse(const std::string& body)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(int statusCode, const std::string& message)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    std::ostringstream json;
    json << "{\"error\":";
    appendJsonString(json, message);
    json << "}";
    response.body = json.str();
    return response;
}

const VdrRecordingNativeArtwork* selectMetadataArtwork(
    const VdrRecordingNativeMetadata& metadata,
    const std::string& kind,
    int index)
{
    if (kind == "preferred") return index == 0 ? &metadata.preferredArtwork : nullptr;
    if (kind == "person")
    {
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.people.size())
            return nullptr;
        return &metadata.people[static_cast<std::size_t>(index)].image;
    }
    if (kind == "gallery")
    {
        if (index < 0 || static_cast<std::size_t>(index) >= metadata.images.size())
            return nullptr;
        return &metadata.images[static_cast<std::size_t>(index)];
    }
    return nullptr;
}

bool localManualPoster(const std::string& path)
{
    if (path.empty()) return false;
    const std::filesystem::path normalized =
        std::filesystem::path(path).lexically_normal();
    return normalized.is_absolute() &&
        normalized != normalized.root_path() &&
        normalized.string().compare(
            0,
            std::string("/var/cache/vdr-suite/recording-metadata/posters/").size(),
            "/var/cache/vdr-suite/recording-metadata/posters/") == 0;
}

std::string manualImageUrl(
    const std::string& backendId,
    const std::string& backendNativeId)
{
    auto encode = [](const std::string& value)
    {
        static const char Hex[] = "0123456789ABCDEF";
        std::string output;
        for (const unsigned char character : value)
        {
            if ((character >= 'A' && character <= 'Z') ||
                (character >= 'a' && character <= 'z') ||
                (character >= '0' && character <= '9') ||
                character == '-' || character == '_' || character == '.' ||
                character == '~')
                output.push_back(static_cast<char>(character));
            else
            {
                output.push_back('%');
                output.push_back(Hex[character >> 4U]);
                output.push_back(Hex[character & 0x0fU]);
            }
        }
        return output;
    };

    return "/api/vdr/recordings/metadata/image?backend=" + encode(backendId) +
        "&backendNativeId=" + encode(backendNativeId) +
        "&kind=preferred&index=0";
}

std::string serializeManualMetadata(
    const ManualRecordingMetadataAssignment& assignment,
    const std::string& backendNativeId)
{
    std::ostringstream json;
    json << "{\"available\":true,\"status\":\"ready\","
         << "\"provider\":\"manual\","
         << "\"mediaType\":";
    appendJsonString(json, assignment.mediaType);
    json << ",\"providerId\":0"
         << ",\"seasonNumber\":" << assignment.seasonNumber
         << ",\"episodeNumber\":" << assignment.episodeNumber
         << ",\"absoluteEpisodeNumber\":0"
         << ",\"runtimeMinutes\":0"
         << ",\"durationDeviationMinutes\":0"
         << ",\"popularity\":0"
         << ",\"voteAverage\":0"
         << ",\"voteCount\":0"
         << ",\"adult\":false"
         << ",\"collectionId\":0"
         << ",\"lastSeason\":0"
         << ",\"title\":";
    appendJsonString(json, assignment.title);
    json << ",\"originalTitle\":";
    appendJsonString(json, assignment.originalTitle);
    json << ",\"episodeName\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.title : std::string{});
    json << ",\"tagline\":\"\""
         << ",\"overview\":";
    appendJsonString(json, assignment.overview);
    json << ",\"releaseDate\":";
    appendJsonString(json, assignment.releaseDate);
    json << ",\"firstAired\":";
    appendJsonString(
        json,
        assignment.mediaType == "episode" ? assignment.releaseDate : std::string{});
    json << ",\"imdbId\":\"\""
         << ",\"statusText\":\"manual\""
         << ",\"collectionName\":\"\""
         << ",\"genres\":[]"
         << ",\"productionCountries\":[]"
         << ",\"networks\":[]"
         << ",\"providerHints\":{\"hd\":0,\"language\":-1}"
         << ",\"preferredArtwork\":{\"available\":"
         << (localManualPoster(assignment.posterReference) ? "true" : "false");
    if (localManualPoster(assignment.posterReference))
    {
        json << ",\"url\":";
        appendJsonString(
            json,
            manualImageUrl(assignment.backendId, backendNativeId));
        json << ",\"width\":0,\"height\":0";
    }
    json << "}"
         << ",\"people\":[]"
         << ",\"images\":[]"
         << ",\"manualAssignment\":{"
         << "\"active\":true,\"revision\":" << assignment.revision
         << ",\"relationshipLocked\":"
         << (assignment.relationshipLocked ? "true" : "false")
         << ",\"providerId\":";
    appendJsonString(json, assignment.providerId);
    json << ",\"externalNamespace\":";
    appendJsonString(json, assignment.externalNamespace);
    json << ",\"externalId\":";
    appendJsonString(json, assignment.externalId);
    json << "}}";
    return json.str();
}

}

VdrRecordingFolderController::VdrRecordingFolderController(
    VdrRecordingCacheRepository& repository,
    NativeMetadataLookup nativeMetadataLookup,
    ManualMetadataLookup manualMetadataLookup,
    std::vector<std::string> metadataImageAllowedRoots)
    : repository_(repository),
      nativeMetadataLookup_(std::move(nativeMetadataLookup)),
      manualMetadataLookup_(std::move(manualMetadataLookup)),
      metadataImageAllowedRoots_(std::move(metadataImageAllowedRoots))
{
    if (metadataImageAllowedRoots_.empty())
    {
        metadataImageAllowedRoots_ = EpgArtworkController::defaultAllowedRoots();
    }
    metadataImageAllowedRoots_.push_back(
        "/var/cache/vdr-suite/recording-metadata/posters");
}

ApiResponse VdrRecordingFolderController::getStatus(
    const std::string& backendId)
{
    const VdrRecordingCacheStatus status = repository_.statusForBackend(backendId);
    std::ostringstream json;
    json << "{\"backendId\":";
    appendJsonString(json, status.backendId);
    json << ",\"state\":";
    appendJsonString(json, status.state);
    json << ",\"cacheReady\":" << (status.totalCount > 0 ? "true" : "false")
         << ",\"totalCount\":" << status.totalCount
         << ",\"startedAt\":";
    appendJsonString(json, status.startedAt);
    json << ",\"finishedAt\":";
    appendJsonString(json, status.finishedAt);
    json << ",\"lastError\":";
    appendJsonString(json, status.lastError);
    json << "}";
    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getFolder(
    const std::string& backendId,
    const std::string& path,
    int limit,
    int offset)
{
    const VdrRecordingFolderPage page = repository_.folderPageForBackend(
        backendId, path, limit, offset);
    std::ostringstream json;
    json << "{\"recordingFolder\":true,\"backendId\":";
    appendJsonString(json, page.backendId);
    json << ",\"path\":";
    appendJsonString(json, page.path);
    json << ",\"parentPath\":";
    appendJsonString(json, page.parentPath);
    json << ",\"cacheState\":";
    appendJsonString(json, page.cacheState);
    json << ",\"cacheReady\":" << (page.cacheReady ? "true" : "false")
         << ",\"totalCount\":" << page.totalCount
         << ",\"folderCount\":" << page.folderCount
         << ",\"recordingCount\":" << page.recordingCount
         << ",\"returnedCount\":" << page.recordings.size()
         << ",\"limit\":" << page.limit
         << ",\"offset\":" << page.offset
         << ",\"folders\":[";
    for (std::size_t index = 0; index < page.folders.size(); ++index)
    {
        if (index > 0) json << ",";
        const auto& folder = page.folders.at(index);
        json << "{\"name\":";
        appendJsonString(json, folder.name);
        json << ",\"path\":";
        appendJsonString(json, folder.path);
        json << ",\"recordingCount\":" << folder.recordingCount
             << ",\"singleRecordingLeaf\":"
             << (folder.singleRecordingLeaf ? "true" : "false");
        if (folder.singleRecordingLeaf)
        {
            json << ",\"singleRecording\":";
            appendRecordingJson(json, folder.singleRecording);
        }
        json << "}";
    }
    json << "],\"recordings\":[";
    for (std::size_t index = 0; index < page.recordings.size(); ++index)
    {
        if (index > 0) json << ",";
        appendRecordingJson(json, page.recordings.at(index));
    }
    json << "]}";
    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getMetadata(
    const std::string& backendId,
    const std::string& backendNativeId) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096)
        return jsonError(400, "backendNativeId is required");

    if (manualMetadataLookup_)
    {
        const ManualRecordingMetadataAssignment manual =
            manualMetadataLookup_(backendId, backendNativeId);
        if (manual.found && manual.relationshipLocked)
            return jsonResponse(serializeManualMetadata(manual, backendNativeId));
    }

    if (!nativeMetadataLookup_)
        return jsonError(503, "recording metadata unavailable");

    return jsonResponse(
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(
            nativeMetadataLookup_(backendId, backendNativeId)));
}

ApiResponse VdrRecordingFolderController::getMetadataImage(
    const std::string& backendId,
    const std::string& backendNativeId,
    const std::string& kind,
    int index) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096 ||
        kind.empty() || index < 0)
        return jsonError(
            400,
            "backendNativeId, kind and non-negative index are required");

    if (kind != "preferred" && kind != "person" && kind != "gallery")
        return jsonError(400, "unsupported recording metadata image kind");

    if (kind == "preferred" && index == 0 && manualMetadataLookup_)
    {
        const ManualRecordingMetadataAssignment manual =
            manualMetadataLookup_(backendId, backendNativeId);
        if (manual.found && manual.relationshipLocked &&
            localManualPoster(manual.posterReference))
        {
            return EpgArtworkController::serveValidatedPath(
                manual.posterReference,
                metadataImageAllowedRoots_);
        }
    }

    if (!nativeMetadataLookup_)
        return jsonError(503, "recording metadata unavailable");

    const VdrRecordingNativeMetadataRecord record =
        nativeMetadataLookup_(backendId, backendNativeId);
    if (!record.exists() || !record.metadata.found)
        return jsonError(404, "recording metadata not found");

    const VdrRecordingNativeArtwork* artwork =
        selectMetadataArtwork(record.metadata, kind, index);
    if (artwork == nullptr || !artwork->available || artwork->path.empty())
        return jsonError(404, "recording metadata image not found");

    return EpgArtworkController::serveValidatedPath(
        artwork->path,
        metadataImageAllowedRoots_);
}
