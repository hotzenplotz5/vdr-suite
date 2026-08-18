#pragma once

#include "MediaCapabilities.h"

#include <map>
#include <optional>
#include <string>

enum class MediaTranscodePresetMode
{
    Auto,
    Superfast,
    Veryfast,
    Faster,
    Fast
};

struct MediaTranscodePolicyConfig
{
    MediaTranscodePresetMode globalPresetMode = MediaTranscodePresetMode::Auto;
    std::optional<MediaTranscodePresetMode> standardPresetMode;
    std::optional<MediaTranscodePresetMode> deinterlacePresetMode;
    std::optional<MediaTranscodePresetMode> uhdSourcePresetMode;
    std::string vaapiDevice = "/dev/dri/renderD128";
    double minimumRealtimeSpeed = 1.25;
};

using MediaTranscodePerformanceSamples = std::map<
    MediaTranscodeWorkload,
    std::map<MediaSoftwareEncoderPreset, double>>;

using MediaHardwareTranscodePerformanceSamples = std::map<
    MediaTranscodeWorkload,
    std::map<MediaVideoEncoderBackend, double>>;

class MediaTranscodePolicy
{
public:
    MediaTranscodePolicy() = default;
    MediaTranscodePolicy(
        MediaTranscodePolicyConfig config,
        MediaTranscodePerformanceSamples samples = {},
        MediaHardwareTranscodePerformanceSamples hardwareSamples = {});

    static MediaTranscodePolicy fromEnvironment();

    MediaPresentationProfile apply(
        const MediaPresentationProfile& profile) const;

    MediaSoftwareEncoderPreset selectPreset(
        MediaTranscodeWorkload workload) const;

    static MediaTranscodePresetMode presetModeFromString(
        const std::string& value,
        bool& valid);

private:
    MediaTranscodePresetMode modeFor(
        MediaTranscodeWorkload workload) const;

    std::optional<MediaSoftwareEncoderPreset> selectMeasuredPreset(
        MediaTranscodeWorkload workload) const;

    bool hardwareMeetsRealtimeThreshold(
        MediaTranscodeWorkload workload,
        MediaVideoEncoderBackend backend) const;

    MediaTranscodePolicyConfig config_;
    MediaTranscodePerformanceSamples samples_;
    MediaHardwareTranscodePerformanceSamples hardwareSamples_;
};
