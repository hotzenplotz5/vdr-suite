#include "RecordingMediaSessionRequestParser.h"

#include <cassert>
#include <string>

namespace
{

const std::string ValidRequest = R"json({
  "backendId":"default",
  "recordingId":"rec-123",
  "capabilities":{
    "protocols":["progressive","hls"],
    "containers":["mpeg-ts","fmp4"],
    "videoCodecs":["h264","mpeg2video"],
    "audioCodecs":["aac","ac3"],
    "supportsByteRanges":true,
    "maxVideoWidth":1920,
    "maxVideoHeight":1080
  }
})json";

void validRequestMapsTypedCapabilities()
{
    const auto result = RecordingMediaSessionRequestParser().parse(ValidRequest);
    assert(result.valid);
    assert(result.reasonCode.empty());
    assert(result.backendId == "default");
    assert(result.recordingId == "rec-123");
    assert(result.capabilities.protocols.size() == 2);
    assert(result.capabilities.protocols[0] == MediaDeliveryProtocol::Progressive);
    assert(result.capabilities.protocols[1] == MediaDeliveryProtocol::Hls);
    assert(result.capabilities.containers.size() == 2);
    assert(result.capabilities.containers[0] == MediaContainer::MpegTs);
    assert(result.capabilities.containers[1] == MediaContainer::Fmp4);
    assert(result.capabilities.videoCodecs.size() == 2);
    assert(result.capabilities.videoCodecs[0] == MediaCodec::H264);
    assert(result.capabilities.videoCodecs[1] == MediaCodec::Mpeg2Video);
    assert(result.capabilities.audioCodecs.size() == 2);
    assert(result.capabilities.audioCodecs[0] == MediaCodec::Aac);
    assert(result.capabilities.audioCodecs[1] == MediaCodec::Ac3);
    assert(result.capabilities.supportsByteRanges);
    assert(result.capabilities.maxVideoWidth == 1920);
    assert(result.capabilities.maxVideoHeight == 1080);
}

void invalidIdentityFailsClosed()
{
    auto missingBackend = RecordingMediaSessionRequestParser().parse(
        R"json({"recordingId":"rec","capabilities":{}})json");
    assert(!missingBackend.valid);
    assert(missingBackend.reasonCode == "invalid_backend_id");

    auto unsafeBackend = RecordingMediaSessionRequestParser().parse(
        R"json({"backendId":"default/../../etc","recordingId":"rec","capabilities":{}})json");
    assert(!unsafeBackend.valid);
    assert(unsafeBackend.reasonCode == "invalid_backend_id");

    auto controlRecording = RecordingMediaSessionRequestParser().parse(
        std::string("{\"backendId\":\"default\",\"recordingId\":\"rec") +
        '\n' + "bad\",\"capabilities\":{}}");
    assert(!controlRecording.valid);
    assert(controlRecording.reasonCode == "invalid_recording_id");
}

void unknownCapabilitiesFailClosed()
{
    std::string body = ValidRequest;
    const auto protocol = body.find("progressive");
    body.replace(protocol, std::string("progressive").size(), "dash");
    auto result = RecordingMediaSessionRequestParser().parse(body);
    assert(!result.valid);
    assert(result.reasonCode == "invalid_media_capabilities");

    body = ValidRequest;
    const auto codec = body.find("h264");
    body.replace(codec, std::string("h264").size(), "vp9");
    result = RecordingMediaSessionRequestParser().parse(body);
    assert(!result.valid);
    assert(result.reasonCode == "invalid_media_capabilities");
}

void malformedScalarsFailClosed()
{
    std::string body = ValidRequest;
    const auto width = body.find("1920");
    body.replace(width, 4, "1920garbage");
    auto result = RecordingMediaSessionRequestParser().parse(body);
    assert(!result.valid);
    assert(result.reasonCode == "invalid_media_capabilities");

    body = ValidRequest;
    const auto ranges = body.find("true");
    body.replace(ranges, 4, "truejunk");
    result = RecordingMediaSessionRequestParser().parse(body);
    assert(!result.valid);
    assert(result.reasonCode == "invalid_media_capabilities");

    body = ValidRequest;
    const auto height = body.find("1080");
    body.replace(height, 4, "20000");
    result = RecordingMediaSessionRequestParser().parse(body);
    assert(!result.valid);
    assert(result.reasonCode == "invalid_media_capabilities");
}

void missingOrEmptyCapabilitiesFailClosed()
{
    auto missing = RecordingMediaSessionRequestParser().parse(
        R"json({"backendId":"default","recordingId":"rec-123"})json");
    assert(!missing.valid);
    assert(missing.reasonCode == "invalid_media_capabilities");

    std::string body = ValidRequest;
    const auto protocols = body.find("\"progressive\",\"hls\"");
    body.replace(protocols, std::string("\"progressive\",\"hls\"").size(), "");
    auto empty = RecordingMediaSessionRequestParser().parse(body);
    assert(!empty.valid);
    assert(empty.reasonCode == "invalid_media_capabilities");
}

} // namespace

int main()
{
    validRequestMapsTypedCapabilities();
    invalidIdentityFailsClosed();
    unknownCapabilitiesFailClosed();
    malformedScalarsFailClosed();
    missingOrEmptyCapabilitiesFailClosed();
    return 0;
}
