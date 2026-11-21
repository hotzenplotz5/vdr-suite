#include "RecordingMediaSessionRequestParser.h"

#include <cassert>

int main()
{
    RecordingMediaSessionRequestParser parser;

    const auto valid = parser.parsePlaybackStatus(
        R"json({"operation":"playback-status","backendId":"default","sessionId":"mediasess_0123456789abcdef"})json");
    assert(valid.valid);
    assert(valid.backendId == "default");
    assert(valid.sessionId == "mediasess_0123456789abcdef");

    const auto seek = parser.parsePlaybackStatus(
        R"json({"operation":"seek","backendId":"default","sessionId":"mediasess_1","positionSeconds":10})json");
    assert(!seek.valid);
    assert(seek.reasonCode == "media_session_playback_status_not_requested");

    const auto stop = parser.parsePlaybackStatus(
        R"json({"operation":"stop","backendId":"default","sessionId":"mediasess_1"})json");
    assert(!stop.valid);
    assert(stop.reasonCode == "media_session_playback_status_not_requested");

    const auto unknown = parser.parsePlaybackStatus(
        R"json({"operation":"delete","backendId":"default","sessionId":"mediasess_1"})json");
    assert(!unknown.valid);
    assert(unknown.reasonCode == "invalid_media_session_operation");

    const auto unsafe = parser.parsePlaybackStatus(
        R"json({"operation":"playback-status","backendId":"default","sessionId":"../bad"})json");
    assert(!unsafe.valid);
    assert(unsafe.reasonCode == "invalid_media_session_id");

    return 0;
}
