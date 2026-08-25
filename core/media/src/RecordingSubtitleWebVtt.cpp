#include "RecordingSubtitleWebVtt.h"

#include <string>

bool RecordingSubtitleWebVtt::supports(MediaSubtitleFormat format)
{
    return format == MediaSubtitleFormat::WebVtt ||
        format == MediaSubtitleFormat::SubRip ||
        format == MediaSubtitleFormat::Ass ||
        format == MediaSubtitleFormat::MovText;
}

RecordingSubtitleWebVttPlan RecordingSubtitleWebVtt::build(
    int sourceSubtitleStreamIndex,
    MediaSubtitleFormat format,
    int streamBasePositionSeconds)
{
    RecordingSubtitleWebVttPlan plan;
    if (sourceSubtitleStreamIndex < 0) {
        plan.reasonCode = "invalid_recording_subtitle_stream_index";
        return plan;
    }
    if (streamBasePositionSeconds < 0) {
        plan.reasonCode = "invalid_recording_subtitle_stream_base";
        return plan;
    }
    if (!supports(format)) {
        plan.reasonCode = "recording_subtitle_format_not_webvtt_convertible";
        return plan;
    }

    plan.argv = {
        "/usr/bin/ffmpeg",
        "-nostdin",
        "-hide_banner",
        "-loglevel", "error"
    };
    if (streamBasePositionSeconds > 0) {
        plan.argv.push_back("-ss");
        plan.argv.push_back(std::to_string(streamBasePositionSeconds));
    }
    plan.argv.insert(plan.argv.end(), {
        "-f", "concat",
        "-safe", "1",
        "-i", "input.ffconcat",
        "-map", "0:s:" + std::to_string(sourceSubtitleStreamIndex),
        "-c:s", "webvtt",
        "-f", "webvtt",
        "pipe:1"
    });
    plan.valid = true;
    return plan;
}
