#include "VdrRecordingQueryResultJsonSerializer.h"

#include "VdrRecordingMetadataJsonSerializer.h"

#include <sstream>
#include <string>

std::string VdrRecordingQueryResultJsonSerializer::serialize(
    const VdrRecordingQueryResult& result) const
{
    return serialize(
        result,
        MetadataOverlaySerializer{});
}

std::string VdrRecordingQueryResultJsonSerializer::serialize(
    const VdrRecordingQueryResult& result,
    const MetadataOverlaySerializer& metadataOverlaySerializer) const
{
    std::ostringstream json;

    json
        << "{"
        << "\"totalCount\":" << result.totalCount() << ","
        << "\"returnedCount\":" << result.returnedCount() << ","
        << "\"limit\":" << result.limit() << ","
        << "\"offset\":" << result.offset() << ","
        << "\"recordings\":[";

    for (std::size_t index = 0;
         index < result.recordings().size();
         ++index)
    {
        const auto& recording =
            result.recordings().at(index);

        if (index > 0)
        {
            json << ",";
        }

        std::string metadata;

        if (metadataOverlaySerializer)
        {
            metadata =
                metadataOverlaySerializer(recording);
        }

        if (metadata.empty())
        {
            metadata =
                VdrRecordingMetadataJsonSerializer::serialize(
                    recording);
        }

        json
            << "{"
            << "\"id\":\"" << recording.id << "\","
            << "\"recordingId\":\"" << recording.id << "\","
            << "\"backendId\":\"" << recording.backendId << "\","
            << "\"title\":\"" << recording.title << "\","
            << "\"path\":\"" << recording.path << "\","
            << "\"recordingPath\":\"" << recording.path << "\","
            << "\"backendNativeId\":\"" << recording.backendNativeId << "\","
            << "\"startTime\":\"" << recording.startTime << "\","
            << "\"durationSeconds\":" << recording.durationSeconds << ","
            << "\"sizeMb\":" << recording.sizeMb << ","
            << "\"metadata\":" << metadata
            << "}";
    }

    json
        << "]"
        << "}";

    return json.str();
}
