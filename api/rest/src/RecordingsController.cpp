#include "RecordingsController.h"

#include "RecordingRepository.h"

#include <sstream>

RecordingsController::RecordingsController(
    RecordingRepository& recordingRepository)
    : recordingRepository_(recordingRepository)
{
}

ApiResponse RecordingsController::getRecordings()
{
    ApiResponse response;

    response.statusCode = 200;
    response.contentType = "application/json";

    const auto recordings =
        recordingRepository_.getAllRecordings();

    std::ostringstream json;

    json << "{";
    json << "\"recordings\":[";

    for (std::size_t i = 0; i < recordings.size(); ++i)
    {
        const auto& recording = recordings[i];

        json << "{";
        json << "\"id\":" << recording.id << ",";
        json << "\"title\":\"" << recording.title << "\",";
        json << "\"subtitle\":\"" << recording.subtitle << "\",";
        json << "\"description\":\"" << recording.description << "\",";
        json << "\"channel\":\"" << recording.channel << "\",";
        json << "\"recordingPath\":\"" << recording.recordingPath << "\",";
        json << "\"recordingFormat\":\"" << recording.recordingFormat << "\"";
        json << "}";

        if (i + 1 < recordings.size())
        {
            json << ",";
        }
    }

    json << "]";
    json << "}";

    response.body =
        json.str();

    return response;
}
