#include "FfmpegHlsCommandBuilder.h"
#include "FfmpegLiveStreamCommandBuilder.h"
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

MediaPresentationProfile hlsProfile()
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

MediaPresentationProfile directCopyProfile()
{
    MediaPresentationProfile value;
    value.available = true;
    value.profileId = "live-progressive-fmp4";
    value.protocol = MediaDeliveryProtocol::Progressive;
    value.container = MediaContainer::Fmp4;
    value.adaptationClass = MediaAdaptationClass::Transcode;
    value.videoAction = MediaTrackAction::Copy;
    value.audioAction = MediaTrackAction::Transcode;
    value.sourceVideoStreamIndex = 0;
    value.sourceAudioStreamIndex = 0;
    value.targetVideoCodec = MediaCodec::H264;
    value.targetAudioCodec = MediaCodec::Aac;
    value.targetVideoWidth = 1920;
    value.targetVideoHeight = 1080;
    value.targetAudioChannels = 2;
    return value;
}

MediaPresentationProfile directInterlacedProfile()
{
    MediaPresentationProfile value = directCopyProfile();
    value.videoAction = MediaTrackAction::Transcode;
    value.targetVideoWidth = 720;
    value.targetVideoHeight = 576;
    value.deinterlaceVideo = true;
    value.videoTranscodeWorkload = MediaTranscodeWorkload::Deinterlace;
    value.videoEncoderBackend = MediaVideoEncoderBackend::SoftwareX264;
    value.videoEncoderPreset = MediaSoftwareEncoderPreset::Superfast;
    return value;
}
}

int main()
{
    const std::string socket = "/run/vdr/vdr-suite-live/lease_live_1.sock";
    const std::string unixUrl =
        "unix:///run/vdr/vdr-suite-live/lease_live_1.sock";
    const std::string invalidQueryUrl =
        unixUrl + "?timeout=5000000&type=stream";

    FfprobeLiveSource probe;
    const auto probePlan = probe.commandPlan(socket);
    assert(probePlan.valid);
    assert(pair(probePlan.argv, "-rw_timeout", "2000000"));
    assert(pair(probePlan.argv, "-analyzeduration", "500000"));
    assert(pair(probePlan.argv, "-probesize", "524288"));
    assert(pair(probePlan.argv, "-f", "mpegts"));
    assert(!contains(probePlan.argv, "-read_intervals"));
    assert(pair(probePlan.argv, "-of", "compact=p=0:nk=0"));
    assert(contains(probePlan.argv, unixUrl));
    assert(!contains(probePlan.argv, invalidQueryUrl));
    assert(!probe.commandPlan("relative.sock").valid);
    assert(!probe.commandPlan("/run/vdr/../tmp/socket").valid);

    const std::string interlacedOutput =
        "codec_type=video|codec_name=h264|width=720|height=576|r_frame_rate=25/1|field_order=tt\n"
        "codec_type=audio|codec_name=mp2|channels=2|tag:language=deu\n";
    const auto source = probe.parse(interlacedOutput);
    assert(source.valid);
    assert(source.source.resourceKind == MediaResourceKind::LiveChannel);
    assert(source.source.container == MediaContainer::MpegTs);
    assert(!source.source.seekable);
    assert(source.source.growing);
    assert(source.source.videoStreams.size() == 1);
    assert(source.source.videoStreams.front().codec == MediaCodec::H264);
    assert(source.source.videoStreams.front().interlaced);
    assert(source.source.videoStreams.front().width == 720);
    assert(source.source.videoStreams.front().height == 576);
    assert(source.source.audioStreams.size() == 1);
    assert(source.source.audioStreams.front().codec == MediaCodec::MpegAudio);
    assert(source.source.audioStreams.front().language == "deu");

    const auto progressiveSource = probe.parse(
        "codec_type=video|codec_name=h264|width=1920|height=1080|r_frame_rate=25/1|field_order=progressive\n"
        "codec_type=audio|codec_name=aac|channels=2|tag:language=deu\n");
    assert(progressiveSource.valid);
    assert(!progressiveSource.source.videoStreams.front().interlaced);

    const auto unknownScan = probe.parse(
        "codec_type=video|codec_name=h264|width=720|height=576|r_frame_rate=25/1|field_order=unknown\n");
    assert(!unknownScan.valid);
    assert(unknownScan.reasonCode == "live_video_scan_type_unknown");

    FfmpegHlsCommandBuilder hlsBuilder;
    auto copy = hlsProfile();
    const auto copyPlan = hlsBuilder.buildLive(copy, socket);
    assert(copyPlan.valid);
    assert(!contains(copyPlan.argv, "-re"));
    assert(!contains(copyPlan.argv, "concat"));
    assert(!contains(copyPlan.argv, "input.ffconcat"));
    assert(pair(copyPlan.argv, "-rw_timeout", "5000000"));
    assert(pair(copyPlan.argv, "-f", "mpegts"));
    assert(contains(copyPlan.argv, unixUrl));
    assert(!contains(copyPlan.argv, invalidQueryUrl));
    assert(pair(copyPlan.argv, "-c:v", "copy"));
    assert(pair(copyPlan.argv, "-c:a", "copy"));
    assert(pair(copyPlan.argv, "-hls_list_size", "8"));
    assert(pair(copyPlan.argv, "-hls_delete_threshold", "2"));
    assert(!contains(copyPlan.argv, "event"));

    auto audioTranscode = hlsProfile();
    audioTranscode.adaptationClass = MediaAdaptationClass::Transcode;
    audioTranscode.audioAction = MediaTrackAction::Transcode;
    audioTranscode.targetAudioCodec = MediaCodec::Aac;
    const auto transcodePlan = hlsBuilder.buildLive(audioTranscode, socket);
    assert(transcodePlan.valid);
    assert(pair(transcodePlan.argv, "-c:v", "copy"));
    assert(pair(transcodePlan.argv, "-c:a", "aac"));

    FfmpegLiveStreamCommandBuilder directBuilder;
    const std::string liveOutput = "/tmp/vdr-suite-live-test.fmp4";
    const auto directCopy = directBuilder.build(
        directCopyProfile(), socket, liveOutput);
    assert(directCopy.valid);
    assert(pair(directCopy.argv, "-c:v", "copy"));
    assert(pair(directCopy.argv, "-c:a", "aac"));
    assert(!contains(directCopy.argv, "libx264"));
    assert(!contains(directCopy.argv, "bwdif=mode=send_frame:parity=auto:deint=all,scale=1920:1080"));
    assert(!contains(directCopy.argv, "master.m3u8"));
    assert(directCopy.argv.back() == liveOutput);

    const auto directInterlaced = directBuilder.build(
        directInterlacedProfile(), socket, liveOutput);
    assert(directInterlaced.valid);
    assert(pair(directInterlaced.argv, "-c:v", "libx264"));
    assert(pair(directInterlaced.argv, "-preset", "superfast"));
    assert(pair(directInterlaced.argv, "-tune", "zerolatency"));
    assert(contains(
        directInterlaced.argv,
        "bwdif=mode=send_frame:parity=auto:deint=all,scale=720:576"));
    assert(pair(directInterlaced.argv, "-pix_fmt", "yuv420p"));
    assert(pair(directInterlaced.argv, "-c:a", "aac"));
    assert(!contains(directInterlaced.argv, "master.m3u8"));
    assert(directInterlaced.argv.back() == liveOutput);

    auto invalidCopy = directCopyProfile();
    invalidCopy.deinterlaceVideo = true;
    const auto invalidCopyPlan = directBuilder.build(
        invalidCopy, socket, liveOutput);
    assert(!invalidCopyPlan.valid);
    assert(invalidCopyPlan.reasonCode == "unsupported_live_video_transformation");

    const auto invalid = hlsBuilder.buildLive(hlsProfile(), "/tmp/a?listen=1");
    assert(!invalid.valid);
    assert(invalid.reasonCode == "invalid_live_source_socket");

    // Private provider addresses remain worker-only details. Public media
    // output is a fixed workspace FIFO and never exposes the VDR socket path.
    assert(directCopy.argv.back().find("/run/vdr") == std::string::npos);

    return 0;
}
