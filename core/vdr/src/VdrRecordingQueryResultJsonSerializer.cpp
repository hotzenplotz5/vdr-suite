#include "VdrRecordingQueryResultJsonSerializer.h"

#include "VdrRecordingMetadataJsonSerializer.h"

#include <algorithm>
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
    const MetadataOverlaySerializer& metadataOverlaySerializer,
    const PublicRecordingIdLookup& publicRecordingIdLookup) const
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

        const std::string publicId = publicRecordingIdLookup
            ? publicRecordingIdLookup(recording) : std::string{};
        json << "{";
        if (publicId.size() == 36 && publicId.compare(0, 4, "rec_") == 0 &&
            std::all_of(publicId.begin() + 4, publicId.end(), [](char value)
            {
                return (value >= '0' && value <= '9') ||
                       (value >= 'a' && value <= 'f');
            }))
        {
            json << "\"publicRecordingId\":\"" << publicId << "\",";
        }

        json
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
