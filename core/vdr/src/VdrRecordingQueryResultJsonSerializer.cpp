#include "VdrRecordingQueryResultJsonSerializer.h"

#include <sstream>

std::string VdrRecordingQueryResultJsonSerializer::serialize(
    const VdrRecordingQueryResult& result) const
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

        json
            << "{"
            << "\"id\":\"" << recording.id << "\","
            << "\"backendId\":\"" << recording.backendId << "\","
            << "\"title\":\"" << recording.title << "\","
            << "\"path\":\"" << recording.path << "\","
            << "\"startTime\":\"" << recording.startTime << "\","
            << "\"durationSeconds\":" << recording.durationSeconds << ","
            << "\"sizeMb\":" << recording.sizeMb
            << "}";
    }

    json
        << "]"
        << "}";

    return json.str();
}
