#include "FfmpegHlsCommandBuilder.h"

#include <string>

namespace
{

FfmpegHlsCommandPlan invalid(const std::string& reasonCode)
{
    FfmpegHlsCommandPlan plan;
    plan.reasonCode = reasonCode;
    return plan;
}

bool appendSelectedTrackMaps(
    std::vector<std::string>& argv,
    const MediaPresentationProfile& profile)
{
    if (profile.videoAction != MediaTrackAction::Omit) {
        if (profile.sourceVideoStreamIndex < 0) {
            return false;
        }
        argv.push_back("-map");
        argv.push_back(
            "0:v:" + std::to_string(profile.sourceVideoStreamIndex) + "?");
    }

    if (profile.audioAction != MediaTrackAction::Omit) {
        if (profile.sourceAudioStreamIndex < 0) {
            return false;
        }
        argv.push_back("-map");
        argv.push_back(
            "0:a:" + std::to_string(profile.sourceAudioStreamIndex) + "?");
    }

    return true;
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
        "/usr/bin/ffmpeg",
        "-nostdin",
        "-hide_banner",
        "-loglevel", "warning",
        "-n",
        // The first vertical intentionally has no arbitrary seek yet. Pace the
        // worker at source rate so one session cannot materialize an entire
        // Recording into its private HLS workspace as fast as the disk allows.
        "-re",
        "-f", "concat",
        "-safe", "1",
        "-i", "input.ffconcat"
    };

    if (!appendSelectedTrackMaps(plan.argv, profile)) {
        return invalid("selected_source_track_missing");
    }

    plan.argv.push_back("-sn");

    bool trackPlanValid = true;
    appendVideoPlan(plan.argv, profile, trackPlanValid);
    appendAudioPlan(plan.argv, profile, trackPlanValid);

    if (!trackPlanValid) {
        return invalid("unsupported_track_transformation");
    }

    if (profile.container == MediaContainer::Fmp4 &&
        profile.videoAction == MediaTrackAction::Copy &&
        profile.targetVideoCodec == MediaCodec::H264) {
        // Some trusted Recording sources carry stale or impossible H.264
        // level_idc metadata. Browsers validate the SPS more strictly than
        // ffmpeg does while remuxing. Let ffmpeg derive the level from the
        // actual bitstream properties without re-encoding the video.
        plan.argv.push_back("-bsf:v");
        plan.argv.push_back("h264_metadata=level=auto");
    }

    if (profile.container == MediaContainer::Fmp4 &&
        profile.audioAction == MediaTrackAction::Copy &&
        profile.targetAudioCodec == MediaCodec::Aac) {
        // VDR MPEG-TS Recordings commonly carry AAC in ADTS framing. MP4/fMP4
        // requires MPEG-4 AudioSpecificConfig instead. Convert the framing
        // metadata while keeping the encoded AAC frames untouched.
        plan.argv.push_back("-bsf:a");
        plan.argv.push_back("aac_adtstoasc");
    }

    plan.argv.insert(
        plan.argv.end(),
        {
            "-f", "hls",
            "-hls_time", "4",
            // Keep a bounded sliding window for this non-seekable first
            // vertical. Later Recording seek is implemented from the logical
            // Recording source/index contract, not by retaining unbounded
            // temporary HLS output.
            "-hls_list_size", "8",
            "-hls_delete_threshold", "2",
            "-hls_flags", "delete_segments+independent_segments+temp_file"
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
