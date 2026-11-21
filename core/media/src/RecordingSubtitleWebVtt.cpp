#include "RecordingSubtitleWebVtt.h"

#include <filesystem>
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
    int streamBasePositionSeconds,
    const std::string& externalSourcePath)
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

    if (!externalSourcePath.empty()) {
        const std::filesystem::path external =
            std::filesystem::path(externalSourcePath).lexically_normal();
        if (!external.is_absolute()) {
            plan.reasonCode = "invalid_recording_subtitle_external_source";
            plan.argv.clear();
            return plan;
        }
        if (format != MediaSubtitleFormat::SubRip) {
            plan.reasonCode = "recording_subtitle_external_format_not_supported";
            plan.argv.clear();
            return plan;
        }
        plan.argv.insert(plan.argv.end(), {
            "-i", external.string(),
            "-map", "0:s:0",
            "-c:s", "webvtt",
            "-f", "webvtt",
            "pipe:1"
        });
    }
    else {
        plan.argv.insert(plan.argv.end(), {
            "-f", "concat",
            "-safe", "1",
            "-i", "input.ffconcat",
            "-map", "0:s:" + std::to_string(sourceSubtitleStreamIndex),
            "-c:s", "webvtt",
            "-f", "webvtt",
            "pipe:1"
        });
    }
    plan.valid = true;
    return plan;
}
