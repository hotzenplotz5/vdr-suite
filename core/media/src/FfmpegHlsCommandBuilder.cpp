#include "FfmpegHlsCommandBuilder.h"

#include <string>

namespace
{

constexpr int MaximumTypedAudioChannels = 32;
constexpr int MaximumTypedVideoDimension = 16384;
constexpr std::size_t MaximumHardwareDevicePathLength = 512;

FfmpegHlsCommandPlan invalid(const std::string& reasonCode)
{
    FfmpegHlsCommandPlan plan;
    plan.reasonCode = reasonCode;
    return plan;
}

const char* x264PresetName(MediaSoftwareEncoderPreset preset)
{
    switch (preset)
    {
    case MediaSoftwareEncoderPreset::Superfast: return "superfast";
    case MediaSoftwareEncoderPreset::Veryfast: return "veryfast";
    case MediaSoftwareEncoderPreset::Faster: return "faster";
    case MediaSoftwareEncoderPreset::Fast: return "fast";
    }
    return nullptr;
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

bool validTargetVideoSize(const MediaPresentationProfile& profile)
{
    return profile.targetVideoWidth >= 2 &&
        profile.targetVideoHeight >= 2 &&
        profile.targetVideoWidth <= MaximumTypedVideoDimension &&
        profile.targetVideoHeight <= MaximumTypedVideoDimension &&
        profile.targetVideoWidth % 2 == 0 &&
        profile.targetVideoHeight % 2 == 0;
}

bool validVaapiDevice(const std::string& value)
{
    return !value.empty() &&
        value.size() <= MaximumHardwareDevicePathLength &&
        value.rfind("/dev/dri/", 0) == 0;
}

bool usesVaapiInput(const MediaPresentationProfile& profile)
{
    return profile.videoAction == MediaTrackAction::Transcode &&
        profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi;
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
        break;
    }

    if (profile.targetVideoCodec != MediaCodec::H264 ||
        !validTargetVideoSize(profile)) {
        valid = false;
        return;
    }

    if (profile.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264) {
        const char* preset = x264PresetName(profile.videoEncoderPreset);
        if (preset == nullptr) {
            valid = false;
            return;
        }
        argv.push_back("-c:v");
        argv.push_back("libx264");
        argv.push_back("-preset");
        argv.push_back(preset);
        argv.push_back("-crf");
        argv.push_back("20");
        argv.push_back("-vf");
        if (profile.deinterlaceVideo) {
            // Convert interlaced Recording video into a progressive browser
            // target before scaling. Keep one output frame per decoded input
            // frame; the server-side transcode policy independently selects
            // an encoder preset with measured real-time headroom.
            argv.push_back(
                "bwdif=mode=send_frame:parity=auto:deint=all,scale=" +
                std::to_string(profile.targetVideoWidth) + ":" +
                std::to_string(profile.targetVideoHeight));
        }
        else {
            argv.push_back(
                "scale=" + std::to_string(profile.targetVideoWidth) +
                ":" + std::to_string(profile.targetVideoHeight));
        }
        // Normalize high-bit-depth HEVC and other source formats to the
        // broadly interoperable 8-bit 4:2:0 H.264 browser target.
        argv.push_back("-pix_fmt");
        argv.push_back("yuv420p");
    }
    else if (profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi) {
        // Phase 65 enables VAAPI only for progressive UHD-source transcodes.
        // Interlaced VAAPI deinterlacing remains outside this maintenance
        // follow-up; such profiles must continue through the software path.
        if (profile.deinterlaceVideo ||
            !validVaapiDevice(profile.videoHardwareDevice)) {
            valid = false;
            return;
        }
        argv.push_back("-c:v");
        argv.push_back("h264_vaapi");
        argv.push_back("-qp");
        argv.push_back("22");
        argv.push_back("-vf");
        argv.push_back(
            "scale_vaapi=w=" + std::to_string(profile.targetVideoWidth) +
            ":h=" + std::to_string(profile.targetVideoHeight) +
            ":format=nv12");
    }
    else {
        valid = false;
        return;
    }

    // HLS cuts on video keyframes. Keep the encoded GOP boundary aligned with
    // the fixed four-second segment target for both x264 and VAAPI.
    argv.push_back("-force_key_frames");
    argv.push_back("expr:gte(t,n_forced*4)");
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
        if (profile.targetAudioCodec != MediaCodec::Aac ||
            profile.targetAudioChannels < 0 ||
            profile.targetAudioChannels > MaximumTypedAudioChannels) {
            valid = false;
            return;
        }
        argv.push_back("-c:a");
        argv.push_back("aac");
        argv.push_back("-b:a");
        argv.push_back("192k");
        if (profile.targetAudioChannels > 0) {
            argv.push_back("-ac");
            argv.push_back(std::to_string(profile.targetAudioChannels));
        }
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
        "-n"
    };

    // Hardware decode options are FFmpeg input options and must be placed
    // before the concat input. Use the exact VAAPI device selected by the
    // calibrated policy; no shell expansion or implicit device discovery is
    // performed by the worker command.
    if (usesVaapiInput(profile)) {
        if (!validVaapiDevice(profile.videoHardwareDevice)) {
            return invalid("unsupported_track_transformation");
        }
        plan.argv.insert(
            plan.argv.end(),
            {
                "-init_hw_device", "vaapi=va:" + profile.videoHardwareDevice,
                "-filter_hw_device", "va",
                "-hwaccel", "vaapi",
                "-hwaccel_device", "va",
                "-hwaccel_output_format", "vaapi"
            });
    }

    plan.argv.insert(
        plan.argv.end(),
        {
            // The first vertical intentionally has no arbitrary seek yet. Pace
            // the worker at source rate so one session cannot materialize an
            // entire Recording into its private HLS workspace as fast as disk
            // or a hardware encoder allows.
            "-re",
            "-f", "concat",
            "-safe", "1",
            "-i", "input.ffconcat"
        });

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

    // Encoded H.264 paths force a keyframe every four seconds and can promise
    // independent HLS segments. Video-copy paths cannot create new keyframes;
    // cutting only on source GOP boundaries can therefore delay publication
    // for many seconds and drain the browser's forward buffer. Split copied
    // video on the fixed time cadence instead, without advertising segment
    // independence that the source bitstream cannot guarantee.
    const char* hlsFlags =
        profile.videoAction == MediaTrackAction::Copy
            ? "delete_segments+split_by_time+temp_file"
            : "delete_segments+independent_segments+temp_file";

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
            "-hls_flags", hlsFlags
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
