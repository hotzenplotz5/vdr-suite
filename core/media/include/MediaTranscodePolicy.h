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

enum class MediaVideoEncoderMode
{
    Auto,
    Software,
    Vaapi
};

struct MediaTranscodePolicyConfig
{
    MediaVideoEncoderMode videoEncoderMode = MediaVideoEncoderMode::Auto;
    MediaTranscodePresetMode globalPresetMode = MediaTranscodePresetMode::Auto;
    std::optional<MediaTranscodePresetMode> standardPresetMode;
    std::optional<MediaTranscodePresetMode> deinterlacePresetMode;
    std::optional<MediaTranscodePresetMode> uhdSourcePresetMode;
    std::string vaapiDevice = "/dev/dri/renderD128";
    bool vaapiAvailable = false;
    bool calibrationProfilePresent = false;
    bool calibrationProfileValid = false;
    double minimumRealtimeSpeed = 1.25;
};

using MediaTranscodePerformanceSamples = std::map<
    MediaTranscodeWorkload,
    std::map<MediaSoftwareEncoderPreset, double>>;

using MediaHardwareTranscodePerformanceSamples = std::map<
    MediaTranscodeWorkload,
    std::map<MediaVideoEncoderBackend, double>>;

struct MediaTranscodePolicyDiagnostics
{
    MediaVideoEncoderMode videoEncoderMode = MediaVideoEncoderMode::Auto;
    bool calibrationProfilePresent = false;
    bool calibrationProfileValid = false;
    double minimumRealtimeSpeed = 1.25;
    bool softwareCalibrated = false;
    bool softwareSuitable = false;
    bool vaapiImplemented = true;
    bool vaapiAvailable = false;
    bool vaapiCalibrated = false;
    bool vaapiSuitable = false;
    bool forcedVaapiBelowThreshold = false;
    std::string vaapiReason;
};

class MediaTranscodePolicy
{
public:
    MediaTranscodePolicy() = default;
    MediaTranscodePolicy(
        MediaTranscodePolicyConfig config,
        MediaTranscodePerformanceSamples samples = {},
        MediaHardwareTranscodePerformanceSamples hardwareSamples = {});

    static MediaTranscodePolicy fromEnvironment(
        std::optional<MediaVideoEncoderMode> managedMode = std::nullopt,
        std::optional<bool> vaapiAvailable = std::nullopt);

    MediaPresentationProfile apply(
        const MediaPresentationProfile& profile) const;

    MediaSoftwareEncoderPreset selectPreset(
        MediaTranscodeWorkload workload) const;

    MediaVideoEncoderMode videoEncoderMode() const
    {
        return config_.videoEncoderMode;
    }

    const std::string& vaapiDevice() const
    {
        return config_.vaapiDevice;
    }

    MediaTranscodePolicyDiagnostics diagnostics() const;

    static MediaTranscodePresetMode presetModeFromString(
        const std::string& value,
        bool& valid);

    static MediaVideoEncoderMode videoEncoderModeFromString(
        const std::string& value,
        bool& valid);

    static const char* videoEncoderModeName(MediaVideoEncoderMode mode);

private:
    MediaTranscodePresetMode modeFor(
        MediaTranscodeWorkload workload) const;

    std::optional<MediaSoftwareEncoderPreset> selectMeasuredPreset(
        MediaTranscodeWorkload workload) const;

    std::optional<double> hardwareSpeed(
        MediaTranscodeWorkload workload,
        MediaVideoEncoderBackend backend) const;

    bool vaapiTransformationSupported(
        const MediaPresentationProfile& profile) const;

    MediaTranscodePolicyConfig config_;
    MediaTranscodePerformanceSamples samples_;
    MediaHardwareTranscodePerformanceSamples hardwareSamples_;
};
