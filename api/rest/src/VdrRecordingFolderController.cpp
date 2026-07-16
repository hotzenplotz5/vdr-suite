#include "VdrRecordingFolderController.h"

#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingMetadataJsonSerializer.h"

#include <sstream>
#include <string>

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

}

VdrRecordingFolderController::VdrRecordingFolderController(
    VdrRecordingCacheRepository& repository)
    : repository_(repository)
{
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
