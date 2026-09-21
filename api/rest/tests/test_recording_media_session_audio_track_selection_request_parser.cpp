#include "RecordingMediaSessionRequestParser.h"

#include <cassert>
#include <string>

namespace
{

const char* Capabilities = R"json(
  "recordingId":"rec-123",
  "capabilities":{
    "protocols":["progressive"],
    "containers":["fmp4"],
    "videoCodecs":["h264"],
    "audioCodecs":["aac"],
    "supportsByteRanges":false,
    "maxVideoWidth":1920,
    "maxVideoHeight":1080,
    "maxAudioChannels":2
  })json";

std::string selectionBody(
    const std::string& trackId = "audio-2",
    const std::string& position = "2530")
{
    return std::string(
        "{\"operation\":\"select-audio-track\","
        "\"backendId\":\"default\","
        "\"sessionId\":\"mediasess_1\","
        "\"audioTrackId\":\"") + trackId + "\"," +
        "\"positionSeconds\":" + position + "," +
        Capabilities + "}";
}

} // namespace

int main()
{
    const auto trackStatus = RecordingMediaSessionRequestParser().parseTrackStatus(
        R"json({"operation":"track-status","backendId":"default","sessionId":"mediasess_1"})json");
    assert(trackStatus.valid);
    assert(trackStatus.backendId == "default");
    assert(trackStatus.sessionId == "mediasess_1");

    const auto valid = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        selectionBody());
    assert(valid.valid);
    assert(valid.reasonCode.empty());
    assert(valid.backendId == "default");
    assert(valid.recordingId == "rec-123");
    assert(valid.sessionId == "mediasess_1");
    assert(valid.audioTrackId == "audio-2");
    assert(valid.positionSeconds == 2530);
    assert(valid.capabilities.protocols.size() == 1);
    assert(valid.capabilities.protocols.front() == MediaDeliveryProtocol::Progressive);
    assert(valid.capabilities.containers.front() == MediaContainer::Fmp4);
    assert(valid.capabilities.audioCodecs.front() == MediaCodec::Aac);

    const auto seek = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"seek","backendId":"default","sessionId":"mediasess_1","positionSeconds":12})json");
    assert(!seek.valid);
    assert(seek.reasonCode == "media_session_audio_track_selection_not_requested");

    const auto statusIsNotSelection =
        RecordingMediaSessionRequestParser().parseAudioTrackSelection(
            R"json({"operation":"track-status","backendId":"default","sessionId":"mediasess_1"})json");
    assert(!statusIsNotSelection.valid);
    assert(statusIsNotSelection.reasonCode ==
        "media_session_audio_track_selection_not_requested");

    const auto malformedTrack = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        selectionBody("pid-1234"));
    assert(!malformedTrack.valid);
    assert(malformedTrack.reasonCode == "invalid_audio_track_id");

    const auto zeroTrack = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        selectionBody("audio-0"));
    assert(!zeroTrack.valid);
    assert(zeroTrack.reasonCode == "invalid_audio_track_id");

    const auto negativePosition = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        selectionBody("audio-1", "-1"));
    assert(!negativePosition.valid);
    assert(negativePosition.reasonCode == "invalid_recording_audio_track_position");

    const auto missingCapabilities = RecordingMediaSessionRequestParser().parseAudioTrackSelection(
        R"json({"operation":"select-audio-track","backendId":"default","recordingId":"rec-123","sessionId":"mediasess_1","audioTrackId":"audio-1","positionSeconds":0})json");
    assert(!missingCapabilities.valid);
    assert(missingCapabilities.reasonCode == "invalid_media_capabilities");

    const auto unsafeSession = RecordingMediaSessionRequestParser().parseTrackStatus(
        R"json({"operation":"track-status","backendId":"default","sessionId":"../bad"})json");
    assert(!unsafeSession.valid);
    assert(unsafeSession.reasonCode == "invalid_media_session_id");

    return 0;
}
