#include "FfprobeRecordingSource.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>

int main()
{
    FfprobeRecordingSource probe;

    {
        const auto plan = probe.commandPlan();
        assert(!plan.argv.empty());
        assert(plan.argv.front() == "/usr/bin/ffprobe");
        assert(std::find(plan.argv.begin(), plan.argv.end(), "/bin/sh") == plan.argv.end());
        assert(std::find(plan.argv.begin(), plan.argv.end(), "input.ffconcat") != plan.argv.end());
    }

    {
        const std::string output =
            "codec_name=h264|codec_type=video|width=1920|height=1080|r_frame_rate=50/1\n"
            "codec_name=ac3|codec_type=audio|channels=6|tag:language=deu\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.reasonCode.empty());
        assert(result.source.resourceKind == MediaResourceKind::Recording);
        assert(result.source.container == MediaContainer::MpegTs);
        assert(result.source.seekable);
        assert(!result.source.growing);
        assert(result.source.videoStreams.size() == 1);
        assert(result.source.audioStreams.size() == 1);
        assert(result.source.videoStreams[0].codec == MediaCodec::H264);
        assert(result.source.videoStreams[0].width == 1920);
        assert(result.source.videoStreams[0].height == 1080);
        assert(std::fabs(result.source.videoStreams[0].framesPerSecond - 50.0) < 0.001);
        assert(result.source.audioStreams[0].codec == MediaCodec::Ac3);
        assert(result.source.audioStreams[0].channels == 6);
        assert(result.source.audioStreams[0].language == "deu");
    }

    {
        const std::string output =
            "codec_name=mpeg2video|codec_type=video|width=720|height=576|r_frame_rate=25/1\n"
            "codec_name=mp2|codec_type=audio|channels=2\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.source.videoStreams[0].codec == MediaCodec::Mpeg2Video);
        assert(result.source.audioStreams[0].codec == MediaCodec::MpegAudio);
    }

    {
        const auto result = probe.parse(
            "codec_name=av1|codec_type=video|width=1920|height=1080|r_frame_rate=25/1\n");
        assert(!result.valid);
        assert(result.reasonCode == "unknown_video_codec");
    }

    {
        const auto result = probe.parse("");
        assert(!result.valid);
        assert(result.reasonCode == "no_media_streams_detected");
    }

    return 0;
}
