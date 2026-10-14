#include "RecordingTrickplayDeviceRequest.h"

#include <algorithm>
#include <string>

RecordingTrickplayDeviceRequest
RecordingTrickplayDeviceRequestParser::parse(
    const std::string& target) const
{
    RecordingTrickplayDeviceRequest response;
    constexpr const char* prefix =
        "/api/v1/recording-playback-sessions/";
    constexpr const char* suffix = "/trickplay?seconds=";
    const std::string start(prefix);
    const std::string ending(suffix);
    if (target.size() > 160 || target.size() <
        start.size() + 35U + ending.size() + 1U ||
        target.compare(0, start.size(), start) != 0 ||
        target.compare(start.size() + 35U, ending.size(), ending) != 0) {
        response.reasonCode = "trickplay_route_invalid";
        return response;
    }

    const std::string id = target.substr(start.size(), 35U);
    if (id.compare(0, 3U, "ms_") != 0 ||
        !std::all_of(id.begin() + 3U, id.end(),
                     [](unsigned char character) {
                         return (character >= '0' && character <= '9') ||
                             (character >= 'a' && character <= 'f');
                     })) {
        response.reasonCode = "trickplay_session_id_invalid";
        return response;
    }

    const std::string seconds = target.substr(
        start.size() + id.size() + ending.size());
    if (seconds.empty() || seconds.size() > 5U ||
        (seconds.size() > 1U && seconds.front() == '0') ||
        !std::all_of(seconds.begin(), seconds.end(),
                     [](unsigned char character) {
                         return character >= '0' && character <= '9';
                     })) {
        response.reasonCode = "trickplay_seconds_invalid";
        return response;
    }
    int value = 0;
    for (char character : seconds) {
        value = value * 10 + (character - '0');
    }
    if (value >= 43200) {
        response.reasonCode = "trickplay_seconds_invalid";
        return response;
    }

    response.valid = true;
    response.sessionId = id;
    response.positionSeconds = value;
    return response;
}
