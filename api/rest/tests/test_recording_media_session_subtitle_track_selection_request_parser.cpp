#include "RecordingMediaSessionRequestParser.h"

#include <cassert>

int main()
{
    const auto valid = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"subtitle-2","streamBasePositionSeconds":125})json");
    assert(valid.valid);
    assert(valid.reasonCode.empty());
    assert(valid.backendId == "default");
    assert(valid.sessionId == "mediasess_1");
    assert(valid.subtitleTrackId == "subtitle-2");
    assert(valid.streamBasePositionSeconds == 125);

    const auto off = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"off","streamBasePositionSeconds":0})json");
    assert(off.valid);
    assert(off.subtitleTrackId == "off");

    const auto audio = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","sessionId":"mediasess_1","audioTrackId":"audio-1"})json");
    assert(!audio.valid);
    assert(audio.reasonCode == "media_session_subtitle_track_selection_not_requested");

    const auto subtitleIsNotAudio = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"subtitle-1","streamBasePositionSeconds":0})json");
    assert(!subtitleIsNotAudio.valid);
    assert(subtitleIsNotAudio.reasonCode ==
        "media_session_audio_track_selection_not_requested");

    const auto subtitleIsNotStatus = RecordingMediaSessionRequestParser().parseTrackStatus(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"subtitle-1","streamBasePositionSeconds":0})json");
    assert(!subtitleIsNotStatus.valid);
    assert(subtitleIsNotStatus.reasonCode == "media_session_track_status_not_requested");

    const auto zero = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"subtitle-0","streamBasePositionSeconds":0})json");
    assert(!zero.valid);
    assert(zero.reasonCode == "invalid_subtitle_track_id");

    const auto pid = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"spid-1234","streamBasePositionSeconds":0})json");
    assert(!pid.valid);
    assert(pid.reasonCode == "invalid_subtitle_track_id");

    const auto negativeBase = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"mediasess_1","subtitleTrackId":"subtitle-1","streamBasePositionSeconds":-1})json");
    assert(!negativeBase.valid);
    assert(negativeBase.reasonCode == "invalid_recording_subtitle_stream_base");

    const auto unsafeSession = RecordingMediaSessionRequestParser().parseSubtitleTrackSelection(
        R"json({"operation":"select-subtitle-track","backendId":"default","sessionId":"../bad","subtitleTrackId":"subtitle-1","streamBasePositionSeconds":0})json");
    assert(!unsafeSession.valid);
    assert(unsafeSession.reasonCode == "invalid_media_session_id");

    return 0;
}
