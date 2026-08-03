#include "VdrRecordingFolderController.h"

#include "EpgArtworkController.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingMetadataJsonSerializer.h"
#include "VdrRecordingNativeMetadataPublicJsonSerializer.h"

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
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\b':
            json << "\\b";
            break;
        case '\f':
            json << "\\f";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            if (static_cast<unsigned char>(character) < 0x20)
            {
                json << "\\u00";
                const char* digits = "0123456789abcdef";
                json << digits[(character >> 4) & 0x0f];
                json << digits[character & 0x0f];
            }
            else
            {
                json << character;
            }
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

ApiResponse jsonResponse(
    const std::string& body)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ApiResponse jsonError(
    int statusCode,
    const std::string& message)
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
    if (kind == "preferred")
    {
        return index == 0 ? &metadata.preferredArtwork : nullptr;
    }

    if (kind == "person")
    {
        if (index < 0 ||
            static_cast<std::size_t>(index) >= metadata.people.size())
        {
            return nullptr;
        }

        return &metadata.people[static_cast<std::size_t>(index)].image;
    }

    if (kind == "gallery")
    {
        if (index < 0 ||
            static_cast<std::size_t>(index) >= metadata.images.size())
        {
            return nullptr;
        }

        return &metadata.images[static_cast<std::size_t>(index)];
    }

    return nullptr;
}

}

VdrRecordingFolderController::VdrRecordingFolderController(
    VdrRecordingCacheRepository& repository,
    NativeMetadataLookup nativeMetadataLookup,
    std::vector<std::string> metadataImageAllowedRoots)
    : repository_(repository),
      nativeMetadataLookup_(std::move(nativeMetadataLookup)),
      metadataImageAllowedRoots_(std::move(metadataImageAllowedRoots))
{
    if (metadataImageAllowedRoots_.empty())
    {
        metadataImageAllowedRoots_ =
            EpgArtworkController::defaultAllowedRoots();
    }
}

ApiResponse VdrRecordingFolderController::getStatus(
    const std::string& backendId)
{
    const VdrRecordingCacheStatus status =
        repository_.statusForBackend(backendId);

    std::ostringstream json;

    json << "{";
    json << "\"backendId\":";
    appendJsonString(json, status.backendId);
    json << ",\"state\":";
    appendJsonString(json, status.state);
    json << ",\"cacheReady\":"
         << (status.totalCount > 0 ? "true" : "false");
    json << ",\"totalCount\":" << status.totalCount;
    json << ",\"startedAt\":";
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
    const VdrRecordingFolderPage page =
        repository_.folderPageForBackend(
            backendId,
            path,
            limit,
            offset);

    std::ostringstream json;

    json << "{";
    json << "\"recordingFolder\":true";
    json << ",\"backendId\":";
    appendJsonString(json, page.backendId);
    json << ",\"path\":";
    appendJsonString(json, page.path);
    json << ",\"parentPath\":";
    appendJsonString(json, page.parentPath);
    json << ",\"cacheState\":";
    appendJsonString(json, page.cacheState);
    json << ",\"cacheReady\":"
         << (page.cacheReady ? "true" : "false");
    json << ",\"totalCount\":" << page.totalCount;
    json << ",\"folderCount\":" << page.folderCount;
    json << ",\"recordingCount\":" << page.recordingCount;
    json << ",\"returnedCount\":" << page.recordings.size();
    json << ",\"limit\":" << page.limit;
    json << ",\"offset\":" << page.offset;

    json << ",\"folders\":[";
    for (std::size_t index = 0; index < page.folders.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        const auto& folder = page.folders.at(index);

        json << "{";
        json << "\"name\":";
        appendJsonString(json, folder.name);
        json << ",\"path\":";
        appendJsonString(json, folder.path);
        json << ",\"recordingCount\":" << folder.recordingCount;
        json << ",\"singleRecordingLeaf\":"
             << (folder.singleRecordingLeaf ? "true" : "false");

        if (folder.singleRecordingLeaf)
        {
            json << ",\"singleRecording\":";
            appendRecordingJson(json, folder.singleRecording);
        }

        json << "}";
    }
    json << "]";

    json << ",\"recordings\":[";
    for (std::size_t index = 0; index < page.recordings.size(); ++index)
    {
        if (index > 0)
        {
            json << ",";
        }

        appendRecordingJson(json, page.recordings.at(index));
    }
    json << "]";

    json << "}";

    return jsonResponse(json.str());
}

ApiResponse VdrRecordingFolderController::getMetadata(
    const std::string& backendId,
    const std::string& backendNativeId) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096)
    {
        return jsonError(400, "backendNativeId is required");
    }

    if (!nativeMetadataLookup_)
    {
        return jsonError(503, "recording metadata unavailable");
    }

    const VdrRecordingNativeMetadataRecord record =
        nativeMetadataLookup_(backendId, backendNativeId);

    return jsonResponse(
        VdrRecordingNativeMetadataPublicJsonSerializer().serialize(record));
}

ApiResponse VdrRecordingFolderController::getMetadataImage(
    const std::string& backendId,
    const std::string& backendNativeId,
    const std::string& kind,
    int index) const
{
    if (backendNativeId.empty() || backendNativeId.size() > 4096 ||
        kind.empty() || index < 0)
    {
        return jsonError(
            400,
            "backendNativeId, kind and non-negative index are required");
    }

    if (kind != "preferred" && kind != "person" && kind != "gallery")
    {
        return jsonError(400, "unsupported recording metadata image kind");
    }

    if (!nativeMetadataLookup_)
    {
        return jsonError(503, "recording metadata unavailable");
    }

    const VdrRecordingNativeMetadataRecord record =
        nativeMetadataLookup_(backendId, backendNativeId);

    if (!record.exists() || !record.metadata.found)
    {
        return jsonError(404, "recording metadata not found");
    }

    const VdrRecordingNativeArtwork* artwork =
        selectMetadataArtwork(record.metadata, kind, index);

    if (artwork == nullptr || !artwork->available || artwork->path.empty())
    {
        return jsonError(404, "recording metadata image not found");
    }

    return EpgArtworkController::serveValidatedPath(
        artwork->path,
        metadataImageAllowedRoots_);
}
