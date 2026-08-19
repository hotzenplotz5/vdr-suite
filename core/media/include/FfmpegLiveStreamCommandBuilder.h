#pragma once

#include "MediaCapabilities.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

struct FfmpegLiveStreamCommandPlan
{
    bool valid = false;
    std::string reasonCode;
    std::vector<std::string> argv;
};

class FfmpegLiveStreamCommandBuilder
{
public:
    FfmpegLiveStreamCommandPlan build(
        const MediaPresentationProfile& profile,
        const std::string& unixSocketPath,
        const std::string& outputPath) const
    {
        FfmpegLiveStreamCommandPlan plan;
        if (!profile.available ||
            profile.protocol != MediaDeliveryProtocol::Progressive ||
            profile.container != MediaContainer::Fmp4) {
            plan.reasonCode = "profile_is_not_live_progressive_fmp4";
            return plan;
        }
        // sockaddr_un.sun_path is 108 bytes on the yaVDR/Linux target and must
        // remain NUL-terminated, so 107 pathname bytes are the hard maximum.
        if (!validAbsolutePath(unixSocketPath, 107)) {
            plan.reasonCode = "invalid_live_source_socket";
            return plan;
        }
        if (!validAbsolutePath(outputPath, 512)) {
            plan.reasonCode = "invalid_live_stream_output";
            return plan;
        }

        plan.argv = {
            "/usr/bin/ffmpeg",
            "-nostdin",
            "-hide_banner",
            "-loglevel", "warning",
            "-y"
        };

        if (usesVaapiInput(profile)) {
            if (!validVaapiDevice(profile.videoHardwareDevice)) {
                plan.reasonCode = "unsupported_live_video_transformation";
                return plan;
            }
            plan.argv.insert(plan.argv.end(), {
                "-init_hw_device", "vaapi=va:" + profile.videoHardwareDevice,
                "-filter_hw_device", "va",
                "-hwaccel", "vaapi",
                "-hwaccel_device", "va",
                "-hwaccel_output_format", "vaapi"
            });
        }

        plan.argv.insert(plan.argv.end(), {
            "-fflags", "+nobuffer",
            "-analyzeduration", "250000",
            "-probesize", "262144",
            "-rw_timeout", "5000000",
            "-f", "mpegts",
            "-i", "unix://" + unixSocketPath
        });

        if (!appendTrackMaps(plan.argv, profile)) {
            plan.reasonCode = "selected_live_track_missing";
            return plan;
        }
        plan.argv.push_back("-sn");

        if (!appendVideoPlan(plan.argv, profile)) {
            plan.reasonCode = "unsupported_live_video_transformation";
            return plan;
        }
        if (!appendAudioPlan(plan.argv, profile)) {
            plan.reasonCode = "unsupported_live_audio_transformation";
            return plan;
        }

        plan.argv.insert(plan.argv.end(), {
            "-f", "mp4",
            "-movflags", "+empty_moov+default_base_moof+frag_keyframe+omit_tfhd_offset",
            "-frag_duration", "250000",
            "-min_frag_duration", "100000",
            "-flush_packets", "1",
            outputPath
        });
        plan.valid = true;
        return plan;
    }

    // Branch-local compatibility overload. New Live-TV runtime code must pass
    // the probed/adapted presentation explicitly so interlaced video can never
    // silently fall back to browser-incompatible stream copy.
    FfmpegLiveStreamCommandPlan build(
        const std::string& unixSocketPath,
        const std::string& outputPath) const
    {
        MediaPresentationProfile profile;
        profile.available = true;
        profile.profileId = "live-progressive-fmp4";
        profile.protocol = MediaDeliveryProtocol::Progressive;
        profile.container = MediaContainer::Fmp4;
        profile.adaptationClass = MediaAdaptationClass::Transcode;
        profile.videoAction = MediaTrackAction::Copy;
        profile.audioAction = MediaTrackAction::Transcode;
        profile.sourceVideoStreamIndex = 0;
        profile.sourceAudioStreamIndex = 0;
        profile.targetVideoCodec = MediaCodec::H264;
        profile.targetAudioCodec = MediaCodec::Aac;
        profile.targetAudioChannels = 2;
        return build(profile, unixSocketPath, outputPath);
    }

private:
    static const char* x264PresetName(MediaSoftwareEncoderPreset preset)
    {
        switch (preset) {
        case MediaSoftwareEncoderPreset::Superfast: return "superfast";
        case MediaSoftwareEncoderPreset::Veryfast: return "veryfast";
        case MediaSoftwareEncoderPreset::Faster: return "faster";
        case MediaSoftwareEncoderPreset::Fast: return "fast";
        }
        return nullptr;
    }

    static bool appendTrackMaps(
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

    static bool validVideoSize(const MediaPresentationProfile& profile)
    {
        return profile.targetVideoWidth >= 2 && profile.targetVideoHeight >= 2 &&
            profile.targetVideoWidth <= 16384 && profile.targetVideoHeight <= 16384 &&
            profile.targetVideoWidth % 2 == 0 && profile.targetVideoHeight % 2 == 0;
    }

    static bool appendVideoPlan(
        std::vector<std::string>& argv,
        const MediaPresentationProfile& profile)
    {
        switch (profile.videoAction) {
        case MediaTrackAction::Omit:
            argv.push_back("-vn");
            return true;
        case MediaTrackAction::Copy:
            if (profile.targetVideoCodec != MediaCodec::H264 || profile.deinterlaceVideo)
                return false;
            argv.insert(argv.end(), {"-c:v", "copy"});
            return true;
        case MediaTrackAction::Transcode:
            break;
        }

        if (profile.targetVideoCodec != MediaCodec::H264 || !validVideoSize(profile))
            return false;

        if (profile.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264) {
            const char* preset = x264PresetName(profile.videoEncoderPreset);
            if (preset == nullptr) return false;
            argv.insert(argv.end(), {
                "-c:v", "libx264",
                "-preset", preset,
                "-tune", "zerolatency",
                "-crf", "20",
                "-vf"
            });
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
            argv.insert(argv.end(), {"-pix_fmt", "yuv420p"});
            return true;
        }

        if (profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi) {
            if (profile.deinterlaceVideo || !validVaapiDevice(profile.videoHardwareDevice))
                return false;
            argv.insert(argv.end(), {
                "-c:v", "h264_vaapi",
                "-qp", "22",
                "-vf",
                "scale_vaapi=w=" + std::to_string(profile.targetVideoWidth) +
                    ":h=" + std::to_string(profile.targetVideoHeight) + ":format=nv12"
            });
            return true;
        }

        return false;
    }

    static bool appendAudioPlan(
        std::vector<std::string>& argv,
        const MediaPresentationProfile& profile)
    {
        switch (profile.audioAction) {
        case MediaTrackAction::Omit:
            argv.push_back("-an");
            return true;
        case MediaTrackAction::Copy:
            if (profile.targetAudioCodec != MediaCodec::Aac) return false;
            argv.insert(argv.end(), {"-c:a", "copy", "-bsf:a", "aac_adtstoasc"});
            return true;
        case MediaTrackAction::Transcode:
            if (profile.targetAudioCodec != MediaCodec::Aac ||
                profile.targetAudioChannels < 0 || profile.targetAudioChannels > 32) {
                return false;
            }
            argv.insert(argv.end(), {"-c:a", "aac", "-b:a", "192k"});
            if (profile.targetAudioChannels > 0) {
                argv.push_back("-ac");
                argv.push_back(std::to_string(profile.targetAudioChannels));
            }
            return true;
        }
        return false;
    }

    static bool usesVaapiInput(const MediaPresentationProfile& profile)
    {
        return profile.videoAction == MediaTrackAction::Transcode &&
            profile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi;
    }

    static bool validVaapiDevice(const std::string& value)
    {
        return !value.empty() && value.size() <= 512 && value.rfind("/dev/dri/", 0) == 0;
    }

    static bool validAbsolutePath(
        const std::string& value,
        std::size_t maximumLength)
    {
        if (value.empty() || value.front() != '/' || value.size() > maximumLength ||
            value.find("..") != std::string::npos || value.find('?') != std::string::npos ||
            value.find('#') != std::string::npos) {
            return false;
        }
        return std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '/' ||
                character == '-' || character == '_' || character == '.' ||
                character == ':';
        });
    }
};
