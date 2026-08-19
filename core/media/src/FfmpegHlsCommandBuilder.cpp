#include "FfmpegHlsCommandBuilder.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

constexpr int MaximumTypedAudioChannels = 32;
constexpr int MaximumTypedVideoDimension = 16384;
constexpr std::size_t MaximumHardwareDevicePathLength = 512;
constexpr std::size_t MaximumLiveSocketPathLength = 100;

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
        if (profile.sourceVideoStreamIndex < 0) return false;
        argv.push_back("-map");
        argv.push_back("0:v:" + std::to_string(profile.sourceVideoStreamIndex) + "?");
    }
    if (profile.audioAction != MediaTrackAction::Omit) {
        if (profile.sourceAudioStreamIndex < 0) return false;
        argv.push_back("-map");
        argv.push_back("0:a:" + std::to_string(profile.sourceAudioStreamIndex) + "?");
    }
    return true;
}

bool validTargetVideoSize(const MediaPresentationProfile& profile)
{
    return profile.targetVideoWidth >= 2 && profile.targetVideoHeight >= 2 &&
        profile.targetVideoWidth <= MaximumTypedVideoDimension &&
        profile.targetVideoHeight <= MaximumTypedVideoDimension &&
        profile.targetVideoWidth % 2 == 0 && profile.targetVideoHeight % 2 == 0;
}

bool validVaapiDevice(const std::string& value)
{
    return !value.empty() && value.size() <= MaximumHardwareDevicePathLength &&
        value.rfind("/dev/dri/", 0) == 0;
}

bool validLiveSocketPath(const std::string& value)
{
    if (value.empty() || value.front() != '/' ||
        value.size() > MaximumLiveSocketPathLength ||
        value.find("..") != std::string::npos ||
        value.find('?') != std::string::npos ||
        value.find('#') != std::string::npos) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) != 0 || character == '/' ||
            character == '-' || character == '_' || character == '.' ||
            character == ':';
    });
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

    if (profile.targetVideoCodec != MediaCodec::H264 || !validTargetVideoSize(profile)) {
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
            argv.push_back(
                "bwdif=mode=send_frame:parity=auto:deint=all,scale=" +
                std::to_string(profile.targetVideoWidth) + ":" +
                std::to_string(profile.targetVideoHeight));
        }
        else {
            argv.push_back(
                "scale=" + std::to_string(profile.targetVideoWidth) + ":" +
                std::to_string(profile.targetVideoHeight));
        }
        argv.push_back("-pix_fmt");
        argv.push_back("yuv420p");
    }
    else if (profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi) {
        if (profile.deinterlaceVideo || !validVaapiDevice(profile.videoHardwareDevice)) {
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
            ":h=" + std::to_string(profile.targetVideoHeight) + ":format=nv12");
    }
    else {
        valid = false;
        return;
    }

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

FfmpegHlsCommandPlan buildPlan(
    const MediaPresentationProfile& profile,
    const std::string* liveSocketPath)
{
    if (!profile.available || profile.protocol != MediaDeliveryProtocol::Hls)
        return invalid("profile_is_not_hls");
    if (profile.container != MediaContainer::Fmp4 &&
        profile.container != MediaContainer::MpegTs)
        return invalid("unsupported_hls_container");
    if (liveSocketPath != nullptr && !validLiveSocketPath(*liveSocketPath))
        return invalid("invalid_live_source_socket");

    FfmpegHlsCommandPlan plan;
    plan.argv = {
        "/usr/bin/ffmpeg", "-nostdin", "-hide_banner",
        "-loglevel", "warning", "-n"
    };

    if (usesVaapiInput(profile)) {
        if (!validVaapiDevice(profile.videoHardwareDevice))
            return invalid("unsupported_track_transformation");
        plan.argv.insert(plan.argv.end(), {
            "-init_hw_device", "vaapi=va:" + profile.videoHardwareDevice,
            "-filter_hw_device", "va",
            "-hwaccel", "vaapi",
            "-hwaccel_device", "va",
            "-hwaccel_output_format", "vaapi"
        });
    }

    if (liveSocketPath == nullptr) {
        plan.argv.insert(plan.argv.end(), {
            "-re", "-f", "concat", "-safe", "1", "-i", "input.ffconcat"
        });
    }
    else {
        // The native provider already emits source-rate MPEG-TS. Do not add
        // -re: doing so would create a second clock and can make live playback
        // lag behind the actual tuner feed. Keep protocol options separate
        // from the Unix pathname: FFmpeg's unix protocol treats the full URL
        // tail as sockaddr_un.sun_path rather than parsing URL query options.
        plan.argv.insert(plan.argv.end(), {
            "-rw_timeout", "5000000",
            "-f", "mpegts",
            "-i", "unix://" + *liveSocketPath
        });
    }

    if (!appendSelectedTrackMaps(plan.argv, profile))
        return invalid("selected_source_track_missing");
    plan.argv.push_back("-sn");

    bool trackPlanValid = true;
    appendVideoPlan(plan.argv, profile, trackPlanValid);
    appendAudioPlan(plan.argv, profile, trackPlanValid);
    if (!trackPlanValid) return invalid("unsupported_track_transformation");

    if (profile.container == MediaContainer::Fmp4 &&
        profile.videoAction == MediaTrackAction::Copy &&
        profile.targetVideoCodec == MediaCodec::H264) {
        plan.argv.push_back("-bsf:v");
        plan.argv.push_back("h264_metadata=level=auto");
    }
    if (profile.container == MediaContainer::Fmp4 &&
        profile.audioAction == MediaTrackAction::Copy &&
        profile.targetAudioCodec == MediaCodec::Aac) {
        plan.argv.push_back("-bsf:a");
        plan.argv.push_back("aac_adtstoasc");
    }

    const char* hlsFlags = profile.videoAction == MediaTrackAction::Copy
        ? "delete_segments+split_by_time+temp_file"
        : "delete_segments+independent_segments+temp_file";
    plan.argv.insert(plan.argv.end(), {
        "-f", "hls", "-hls_time", "4",
        "-hls_list_size", "8", "-hls_delete_threshold", "2",
        "-hls_flags", hlsFlags
    });

    if (profile.container == MediaContainer::Fmp4) {
        plan.argv.insert(plan.argv.end(), {
            "-hls_segment_type", "fmp4",
            "-hls_fmp4_init_filename", "init.mp4",
            "-hls_segment_filename", "segment-%06d.m4s"
        });
    }
    else {
        plan.argv.insert(plan.argv.end(), {
            "-hls_segment_filename", "segment-%06d.ts"
        });
    }

    plan.argv.push_back("master.m3u8");
    plan.valid = true;
    return plan;
}

} // namespace

FfmpegHlsCommandPlan FfmpegHlsCommandBuilder::build(
    const MediaPresentationProfile& profile) const
{
    return buildPlan(profile, nullptr);
}

FfmpegHlsCommandPlan FfmpegHlsCommandBuilder::buildLive(
    const MediaPresentationProfile& profile,
    const std::string& unixSocketPath) const
{
    return buildPlan(profile, &unixSocketPath);
}
