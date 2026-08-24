#include "RecordingMediaSessionRequestParser.h"

#include <cassert>
#include <string>

int main()
{
    const auto valid = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"mediasess_1","audioTrackId":"audio-2","positionSeconds":2530})json");
    assert(valid.valid);
    assert(valid.reasonCode.empty());
    assert(valid.backendId == "default");
    assert(valid.sessionId == "mediasess_1");
    assert(valid.audioTrackId == "audio-2");
    assert(valid.positionSeconds == 2530);

    const auto seek = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"seek","backendId":"default","sessionId":"mediasess_1","positionSeconds":12})json");
    assert(!seek.valid);
    assert(seek.reasonCode == "media_session_audio_track_selection_not_requested");

    const auto malformedTrack = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"mediasess_1","audioTrackId":"pid-1234","positionSeconds":12})json");
    assert(!malformedTrack.valid);
    assert(malformedTrack.reasonCode == "invalid_audio_track_id");

    const auto zeroTrack = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"mediasess_1","audioTrackId":"audio-0","positionSeconds":12})json");
    assert(!zeroTrack.valid);
    assert(zeroTrack.reasonCode == "invalid_audio_track_id");

    const auto negativePosition = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"mediasess_1","audioTrackId":"audio-1","positionSeconds":-1})json");
    assert(!negativePosition.valid);
    assert(negativePosition.reasonCode == "invalid_recording_audio_track_position");

    const auto unsafeSession = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"../bad","audioTrackId":"audio-1","positionSeconds":0})json");
    assert(!unsafeSession.valid);
    assert(unsafeSession.reasonCode == "invalid_media_session_id");

    return 0;
}
