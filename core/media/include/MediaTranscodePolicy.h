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
    double minimumRealtimeSpeed = 1.25;
};

using MediaTranscodePerformanceSamples = std::map<
    MediaTranscodeWorkload,
    std::map<MediaSoftwareEncoderPreset, double>>;

class MediaTranscodePolicy
{
public:
    MediaTranscodePolicy() = default;
    MediaTranscodePolicy(
        MediaTranscodePolicyConfig config,
        MediaTranscodePerformanceSamples samples = {});

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

    MediaTranscodePolicyConfig config_;
    MediaTranscodePerformanceSamples samples_;
};
