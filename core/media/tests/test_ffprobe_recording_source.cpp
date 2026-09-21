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
        assert(std::any_of(plan.argv.begin(), plan.argv.end(), [](const std::string& value) {
            return value.find("field_order") != std::string::npos;
        }));
        assert(std::any_of(plan.argv.begin(), plan.argv.end(), [](const std::string& value) {
            return value.find("channel_layout") != std::string::npos &&
                value.find("stream_disposition") != std::string::npos &&
                value.find("title") != std::string::npos;
        }));
    }

    {
        const std::string output =
            "codec_name=h264|codec_type=video|width=1920|height=1080|r_frame_rate=50/1|field_order=tt\n"
            "codec_name=ac3|codec_type=audio|channels=6|channel_layout=5.1(side)|disposition:default=1|disposition:original=1|disposition:comment=0|disposition:visual_impaired=0|disposition:hearing_impaired=0|tag:language=deu|tag:title=Deutsch 5.1\n"
            "codec_name=dvb_subtitle|codec_type=subtitle|disposition:default=0|disposition:forced=1|disposition:hearing_impaired=0|tag:language=deu|tag:title=Deutsch forced\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.reasonCode.empty());
        assert(result.source.resourceKind == MediaResourceKind::Recording);
        assert(result.source.container == MediaContainer::MpegTs);
        assert(result.source.seekable);
        assert(!result.source.growing);
        assert(result.source.videoStreams.size() == 1);
        assert(result.source.audioStreams.size() == 1);
        assert(result.source.subtitleStreams.size() == 1);
        assert(result.source.videoStreams[0].codec == MediaCodec::H264);
        assert(result.source.videoStreams[0].width == 1920);
        assert(result.source.videoStreams[0].height == 1080);
        assert(std::fabs(result.source.videoStreams[0].framesPerSecond - 50.0) < 0.001);
        assert(result.source.videoStreams[0].interlaced);
        const auto& audio = result.source.audioStreams[0];
        assert(audio.codec == MediaCodec::Ac3);
        assert(audio.channels == 6);
        assert(audio.language == "deu");
        assert(audio.label == "Deutsch 5.1");
        assert(audio.channelLayout == "5.1(side)");
        assert(audio.defaultTrack);
        assert(audio.original);
        assert(!audio.commentary);
        assert(!audio.descriptive);
        assert(!audio.hearingImpaired);
        const auto& subtitle = result.source.subtitleStreams[0];
        assert(subtitle.format == MediaSubtitleFormat::Dvb);
        assert(subtitle.language == "deu");
        assert(subtitle.label == "Deutsch forced");
        assert(!subtitle.defaultTrack);
        assert(subtitle.forced);
        assert(!subtitle.hearingImpaired);
    }

    {
        const std::string output =
            "codec_name=mpeg2video|codec_type=video|width=720|height=576|r_frame_rate=25/1\n"
            "codec_name=mp2|codec_type=audio|channels=2\n"
            "codec_name=dvb_teletext|codec_type=subtitle|tag:language=eng\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.source.videoStreams[0].codec == MediaCodec::Mpeg2Video);
        assert(!result.source.videoStreams[0].interlaced);
        assert(result.source.audioStreams[0].codec == MediaCodec::MpegAudio);
        assert(result.source.audioStreams[0].language.empty());
        assert(result.source.audioStreams[0].label.empty());
        assert(result.source.audioStreams[0].channelLayout.empty());
        assert(!result.source.audioStreams[0].defaultTrack);
        assert(result.source.subtitleStreams.size() == 1);
        assert(result.source.subtitleStreams[0].format == MediaSubtitleFormat::Teletext);
        assert(result.source.subtitleStreams[0].language == "eng");
    }

    {
        const std::string output =
            "codec_name=h264|codec_type=video|width=1280|height=688|r_frame_rate=24000/1001|field_order=progressive\n"
            "codec_name=dts|codec_type=audio|channels=6|disposition:comment=1|tag:language=ger\n"
            "codec_name=dts|codec_type=audio|channels=6|disposition:visual_impaired=1|disposition:hearing_impaired=1|tag:language=eng\n"
            "codec_name=webvtt|codec_type=subtitle|disposition:default=1|disposition:forced=0|disposition:hearing_impaired=1|tag:language=eng\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.reasonCode.empty());
        assert(!result.source.videoStreams[0].interlaced);
        assert(result.source.audioStreams.size() == 2);
        assert(result.source.audioStreams[0].codec == MediaCodec::Dts);
        assert(result.source.audioStreams[0].channels == 6);
        assert(result.source.audioStreams[0].language == "ger");
        assert(result.source.audioStreams[0].commentary);
        assert(!result.source.audioStreams[0].descriptive);
        assert(result.source.audioStreams[1].codec == MediaCodec::Dts);
        assert(result.source.audioStreams[1].channels == 6);
        assert(result.source.audioStreams[1].language == "eng");
        assert(result.source.audioStreams[1].descriptive);
        assert(result.source.audioStreams[1].hearingImpaired);
        assert(result.source.subtitleStreams.size() == 1);
        assert(result.source.subtitleStreams[0].format == MediaSubtitleFormat::WebVtt);
        assert(result.source.subtitleStreams[0].defaultTrack);
        assert(!result.source.subtitleStreams[0].forced);
        assert(result.source.subtitleStreams[0].hearingImpaired);
    }

    {
        const std::string output =
            "codec_name=h264|codec_type=video|width=1920|height=1080|r_frame_rate=25/1\n"
            "codec_name=vorbis|codec_type=audio|channels=6|tag:language=eng\n"
            "codec_name=aac|codec_type=audio|channels=2|tag:language=deu\n"
            "codec_name=unknown_subtitle|codec_type=subtitle|tag:language=und\n";

        const auto result = probe.parse(output);
        assert(result.valid);
        assert(result.reasonCode.empty());
        assert(result.source.audioStreams.size() == 2);
        assert(result.source.audioStreams[0].codec == MediaCodec::Unknown);
        assert(result.source.audioStreams[1].codec == MediaCodec::Aac);
        assert(result.source.subtitleStreams.size() == 1);
        assert(result.source.subtitleStreams[0].format == MediaSubtitleFormat::Unknown);
    }

    {
        const auto result = probe.parse(
            "codec_name=h264|codec_type=video|width=1920|height=1080|r_frame_rate=25/1\n"
            "codec_name=vorbis|codec_type=audio|channels=6\n");
        assert(!result.valid);
        assert(result.reasonCode == "unknown_audio_codec");
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
