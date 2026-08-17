#include "FfmpegHlsCommandBuilder.h"

namespace
{

FfmpegHlsCommandPlan invalid(const std::string& reasonCode)
{
    FfmpegHlsCommandPlan plan;
    plan.reasonCode = reasonCode;
    return plan;
}

void appendVideoPlan(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile,
    bool& valid)
{
    switch (profile.videoAction) {
    case MediaTrackAction::Omit:
        argv.push_back("-vn");
        return;
    case MediaTrackAction::Copy:
        argv.push_back("-c:v");
        argv.push_back("copy");
        return;
    case MediaTrackAction::Transcode:
        if (profile.targetVideoCodec != MediaCodec::H264) {
            valid = false;
            return;
        }
        argv.push_back("-c:v");
        argv.push_back("libx264");
        argv.push_back("-preset");
        argv.push_back("veryfast");
        argv.push_back("-crf");
        argv.push_back("20");
        return;
    }

    valid = false;
}

void appendAudioPlan(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile,
    bool& valid)
{
    switch (profile.audioAction) {
    case MediaTrackAction::Omit:
        argv.push_back("-an");
        return;
    case MediaTrackAction::Copy:
        argv.push_back("-c:a");
        argv.push_back("copy");
        return;
    case MediaTrackAction::Transcode:
        if (profile.targetAudioCodec != MediaCodec::Aac) {
            valid = false;
            return;
        }
        argv.push_back("-c:a");
        argv.push_back("aac");
        argv.push_back("-b:a");
        argv.push_back("192k");
        return;
    }

    valid = false;
}

} // namespace

FfmpegHlsCommandPlan FfmpegHlsCommandBuilder::build(
    const MediaPresentationProfile& profile) const
{
    if (!profile.available ||
        profile.protocol != MediaDeliveryProtocol::Hls) {
        return invalid("profile_is_not_hls");
    }

    if (profile.container != MediaContainer::Fmp4 &&
        profile.container != MediaContainer::MpegTs) {
        return invalid("unsupported_hls_container");
    }

    FfmpegHlsCommandPlan plan;
    plan.argv = {
        "ffmpeg",
        "-nostdin",
        "-hide_banner",
        "-loglevel", "warning",
        "-n",
        "-f", "concat",
        "-safe", "1",
        "-i", "input.ffconcat",
        "-map", "0:v:0?",
        "-map", "0:a:0?",
        "-sn"
    };

    bool trackPlanValid = true;
    appendVideoPlan(plan.argv, profile, trackPlanValid);
    appendAudioPlan(plan.argv, profile, trackPlanValid);

    if (!trackPlanValid) {
        return invalid("unsupported_track_transformation");
    }

    plan.argv.insert(
        plan.argv.end(),
        {
            "-f", "hls",
            "-hls_time", "4",
            "-hls_list_size", "0",
            "-hls_playlist_type", "event"
        });

    if (profile.container == MediaContainer::Fmp4) {
        plan.argv.insert(
            plan.argv.end(),
            {
                "-hls_segment_type", "fmp4",
                "-hls_fmp4_init_filename", "init.mp4",
                "-hls_segment_filename", "segment-%06d.m4s"
            });
    }
    else {
        plan.argv.insert(
            plan.argv.end(),
            {
                "-hls_segment_filename", "segment-%06d.ts"
            });
    }

    plan.argv.push_back("master.m3u8");
    plan.valid = true;
    return plan;
}
