#include "RecordingMediaSessionAudioTrackPreference.h"

#include <cassert>

int main()
{
    const RecordingMediaSessionAudioTrackPreferenceParser parser;

    const auto absent = parser.parse(
        R"json({"backendId":"default","recordingId":"42"})json");
    assert(absent.valid);
    assert(absent.reasonCode.empty());
    assert(absent.audioTrackId.empty());

    const auto selected = parser.parse(
        R"json({"backendId":"default","recordingId":"42","audioTrackId":"audio-2"})json");
    assert(selected.valid);
    assert(selected.reasonCode.empty());
    assert(selected.audioTrackId == "audio-2");

    const auto pid = parser.parse(
        R"json({"audioTrackId":"pid-123"})json");
    assert(!pid.valid);
    assert(pid.reasonCode == "invalid_audio_track_id");

    const auto zero = parser.parse(
        R"json({"audioTrackId":"audio-0"})json");
    assert(!zero.valid);
    assert(zero.reasonCode == "invalid_audio_track_id");

    const auto malformed = parser.parse(
        R"json({"audioTrackId":2})json");
    assert(!malformed.valid);
    assert(malformed.reasonCode == "invalid_audio_track_id");

    return 0;
}
