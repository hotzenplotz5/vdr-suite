#include "FfmpegHlsCommandBuilder.h"
#include "FfprobeLiveSource.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{
bool contains(const std::vector<std::string>& values, const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool pair(const std::vector<std::string>& values,
          const std::string& option,
          const std::string& value)
{
    for (std::size_t i = 0; i + 1 < values.size(); ++i)
        if (values[i] == option && values[i + 1] == value) return true;
    return false;
}

MediaPresentationProfile profile()
{
    MediaPresentationProfile value;
    value.available = true;
    value.protocol = MediaDeliveryProtocol::Hls;
    value.container = MediaContainer::Fmp4;
    value.adaptationClass = MediaAdaptationClass::Remux;
    value.videoAction = MediaTrackAction::Copy;
    value.audioAction = MediaTrackAction::Copy;
    value.sourceVideoStreamIndex = 0;
    value.sourceAudioStreamIndex = 0;
    value.targetVideoCodec = MediaCodec::H264;
    value.targetAudioCodec = MediaCodec::Aac;
    value.targetVideoWidth = 1920;
    value.targetVideoHeight = 1080;
    value.targetAudioChannels = 2;
    return value;
}
}

int main()
{
    const std::string socket = "/run/vdr/vdr-suite-live/lease_live_1.sock";

    FfprobeLiveSource probe;
    const auto probePlan = probe.commandPlan(socket);
    assert(probePlan.valid);
    assert(pair(probePlan.argv, "-f", "mpegts"));
    assert(pair(probePlan.argv, "-read_intervals", "%+3"));
    assert(contains(
        probePlan.argv,
        "unix:///run/vdr/vdr-suite-live/lease_live_1.sock?timeout=5000000&type=stream"));
    assert(!probe.commandPlan("relative.sock").valid);
    assert(!probe.commandPlan("/run/vdr/../tmp/socket").valid);

    const std::string output =
        "[STREAM]\nindex=0\ncodec_name=h264\ncodec_type=video\n"
        "width=1920\nheight=1080\navg_frame_rate=25/1\nfield_order=tt\n[/STREAM]\n"
        "[STREAM]\nindex=1\ncodec_name=mp2\ncodec_type=audio\nchannels=2\n[/STREAM]\n"
        "[FORMAT]\nformat_name=mpegts\n[/FORMAT]\n";
    const auto source = probe.parse(output);
    assert(source.valid);
    assert(source.source.resourceKind == MediaResourceKind::LiveChannel);
    assert(source.source.container == MediaContainer::MpegTs);
    assert(!source.source.seekable);
    assert(!source.source.growing);
    assert(source.source.videoStreams.size() == 1);
    assert(source.source.videoStreams.front().codec == MediaCodec::H264);
    assert(source.source.videoStreams.front().interlaced);
    assert(source.source.audioStreams.size() == 1);
    assert(source.source.audioStreams.front().codec == MediaCodec::MpegAudio);

    FfmpegHlsCommandBuilder builder;
    auto copy = profile();
    const auto copyPlan = builder.buildLive(copy, socket);
    assert(copyPlan.valid);
    assert(!contains(copyPlan.argv, "-re"));
    assert(!contains(copyPlan.argv, "concat"));
    assert(!contains(copyPlan.argv, "input.ffconcat"));
    assert(pair(copyPlan.argv, "-f", "mpegts"));
    assert(contains(
        copyPlan.argv,
        "unix:///run/vdr/vdr-suite-live/lease_live_1.sock?timeout=5000000&type=stream"));
    assert(pair(copyPlan.argv, "-c:v", "copy"));
    assert(pair(copyPlan.argv, "-c:a", "copy"));
    assert(pair(copyPlan.argv, "-hls_list_size", "8"));
    assert(pair(copyPlan.argv, "-hls_delete_threshold", "2"));
    assert(!contains(copyPlan.argv, "event"));

    auto audioTranscode = profile();
    audioTranscode.adaptationClass = MediaAdaptationClass::Transcode;
    audioTranscode.audioAction = MediaTrackAction::Transcode;
    audioTranscode.targetAudioCodec = MediaCodec::Aac;
    const auto transcodePlan = builder.buildLive(audioTranscode, socket);
    assert(transcodePlan.valid);
    assert(pair(transcodePlan.argv, "-c:v", "copy"));
    assert(pair(transcodePlan.argv, "-c:a", "aac"));
    assert(!contains(transcodePlan.argv, "-re"));

    const auto invalid = builder.buildLive(profile(), "/tmp/a?listen=1");
    assert(!invalid.valid);
    assert(invalid.reasonCode == "invalid_live_source_socket");

    // The private provider address is an input-only worker detail. Output
    // artifacts are fixed workspace-relative names and cannot leak the socket.
    assert(copyPlan.argv.back() == "master.m3u8");
    assert(copyPlan.argv.back().find("/run/vdr") == std::string::npos);

    return 0;
}
