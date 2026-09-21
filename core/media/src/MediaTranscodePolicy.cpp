#include "MediaTranscodePolicy.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace
{

constexpr const char* DefaultPerformanceProfilePath =
    "/var/lib/vdr-suite/media-transcode-performance.conf";
constexpr std::size_t MaximumHardwareDevicePathLength = 512;

std::string trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool validVaapiDevice(const std::string& value)
{
    return !value.empty() &&
        value.size() <= MaximumHardwareDevicePathLength &&
        value.rfind("/dev/dri/", 0) == 0;
}

MediaSoftwareEncoderPreset presetForMode(MediaTranscodePresetMode mode)
{
    switch (mode) {
    case MediaTranscodePresetMode::Superfast:
        return MediaSoftwareEncoderPreset::Superfast;
    case MediaTranscodePresetMode::Veryfast:
        return MediaSoftwareEncoderPreset::Veryfast;
    case MediaTranscodePresetMode::Faster:
        return MediaSoftwareEncoderPreset::Faster;
    case MediaTranscodePresetMode::Fast:
        return MediaSoftwareEncoderPreset::Fast;
    case MediaTranscodePresetMode::Auto:
        break;
    }
    return MediaSoftwareEncoderPreset::Veryfast;
}

MediaSoftwareEncoderPreset conservativeFallback(MediaTranscodeWorkload workload)
{
    return workload == MediaTranscodeWorkload::Deinterlace
        ? MediaSoftwareEncoderPreset::Superfast
        : MediaSoftwareEncoderPreset::Veryfast;
}

bool workloadFromString(
    const std::string& value,
    MediaTranscodeWorkload& workload)
{
    if (value == "standard") {
        workload = MediaTranscodeWorkload::Standard;
        return true;
    }
    if (value == "deinterlace") {
        workload = MediaTranscodeWorkload::Deinterlace;
        return true;
    }
    if (value == "uhd-source") {
        workload = MediaTranscodeWorkload::UhdSource;
        return true;
    }
    return false;
}

bool presetFromString(
    const std::string& value,
    MediaSoftwareEncoderPreset& preset)
{
    if (value == "superfast") {
        preset = MediaSoftwareEncoderPreset::Superfast;
        return true;
    }
    if (value == "veryfast") {
        preset = MediaSoftwareEncoderPreset::Veryfast;
        return true;
    }
    if (value == "faster") {
        preset = MediaSoftwareEncoderPreset::Faster;
        return true;
    }
    if (value == "fast") {
        preset = MediaSoftwareEncoderPreset::Fast;
        return true;
    }
    return false;
}

bool backendFromString(
    const std::string& value,
    MediaVideoEncoderBackend& backend)
{
    if (value == "vaapi") {
        backend = MediaVideoEncoderBackend::Vaapi;
        return true;
    }
    if (value == "qsv") {
        backend = MediaVideoEncoderBackend::Qsv;
        return true;
    }
    if (value == "nvenc") {
        backend = MediaVideoEncoderBackend::Nvenc;
        return true;
    }
    return false;
}

bool positiveFiniteDouble(const std::string& text, double& value)
{
    if (text.empty()) return false;
    char* end = nullptr;
    const double parsed = std::strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' ||
        !std::isfinite(parsed) || parsed <= 0.0 || parsed > 100.0) {
        return false;
    }
    value = parsed;
    return true;
}

struct LoadedPerformanceProfile
{
    bool present = false;
    bool valid = false;
    MediaTranscodePerformanceSamples software;
    MediaHardwareTranscodePerformanceSamples hardware;
    std::string vaapiDevice;
};

LoadedPerformanceProfile loadPerformanceProfile(const std::string& path)
{
    LoadedPerformanceProfile profile;
    std::ifstream input(path);
    if (!input.is_open()) return profile;
    profile.present = true;

    std::vector<std::string> lines;
    std::string line;
    bool supportedVersion = false;
    while (std::getline(input, line)) {
        lines.push_back(line);
        const std::string normalized = trim(line);
        if (normalized == "version=3" || normalized == "version=4") {
            supportedVersion = true;
        }
    }

    // Versions 1 and 2 were intentionally retired after real yaVDR evidence.
    // v3 remains valid for software calibration and v4 can additionally carry
    // hardware measurements and the render device used for those measurements.
    if (!supportedVersion) return profile;
    profile.valid = true;

    for (const std::string& rawLine : lines) {
        line = trim(rawLine);
        if (line.empty() || line.front() == '#') continue;

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || equals == 0) continue;

        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));
        if (key == "version") continue;
        if (key == "vaapi.device") {
            if (validVaapiDevice(value)) profile.vaapiDevice = value;
            continue;
        }

        const std::size_t dot = key.find('.');
        if (dot == std::string::npos || dot == 0 || dot + 1 >= key.size()) {
            continue;
        }

        MediaTranscodeWorkload workload = MediaTranscodeWorkload::None;
        double speed = 0.0;
        if (!workloadFromString(key.substr(0, dot), workload) ||
            !positiveFiniteDouble(value, speed)) {
            continue;
        }

        const std::string implementation = key.substr(dot + 1);
        MediaSoftwareEncoderPreset preset = MediaSoftwareEncoderPreset::Veryfast;
        if (presetFromString(implementation, preset)) {
            profile.software[workload][preset] = speed;
            continue;
        }

        MediaVideoEncoderBackend backend = MediaVideoEncoderBackend::SoftwareX264;
        if (backendFromString(implementation, backend)) {
            profile.hardware[workload][backend] = speed;
        }
    }
    return profile;
}

std::optional<MediaTranscodePresetMode> optionalModeFromEnvironment(
    const char* name)
{
    const char* raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0') return std::nullopt;
    bool valid = false;
    const MediaTranscodePresetMode mode =
        MediaTranscodePolicy::presetModeFromString(raw, valid);
    return valid ? std::optional<MediaTranscodePresetMode>(mode) : std::nullopt;
}

bool anySoftwareCalibration(const MediaTranscodePerformanceSamples& samples)
{
    for (const auto& workload : samples) {
        if (!workload.second.empty()) return true;
    }
    return false;
}

bool anySoftwareSuitable(
    const MediaTranscodePerformanceSamples& samples,
    double minimumRealtimeSpeed)
{
    for (const auto& workload : samples) {
        for (const auto& sample : workload.second) {
            if (sample.second >= minimumRealtimeSpeed) return true;
        }
    }
    return false;
}

bool anyVaapiCalibration(
    const MediaHardwareTranscodePerformanceSamples& samples)
{
    for (const auto& workload : samples) {
        const auto sample = workload.second.find(MediaVideoEncoderBackend::Vaapi);
        if (sample != workload.second.end()) return true;
    }
    return false;
}

bool anyVaapiSuitable(
    const MediaHardwareTranscodePerformanceSamples& samples,
    double minimumRealtimeSpeed)
{
    for (const auto& workload : samples) {
        const auto sample = workload.second.find(MediaVideoEncoderBackend::Vaapi);
        if (sample != workload.second.end() &&
            sample->second >= minimumRealtimeSpeed) {
            return true;
        }
    }
    return false;
}

bool anyVaapiBelowThreshold(
    const MediaHardwareTranscodePerformanceSamples& samples,
    double minimumRealtimeSpeed)
{
    for (const auto& workload : samples) {
        const auto sample = workload.second.find(MediaVideoEncoderBackend::Vaapi);
        if (sample != workload.second.end() &&
            sample->second < minimumRealtimeSpeed) {
            return true;
        }
    }
    return false;
}

} // namespace

MediaTranscodePolicy::MediaTranscodePolicy(
    MediaTranscodePolicyConfig config,
    MediaTranscodePerformanceSamples samples,
    MediaHardwareTranscodePerformanceSamples hardwareSamples)
    : config_(std::move(config)),
      samples_(std::move(samples)),
      hardwareSamples_(std::move(hardwareSamples))
{
    if (!std::isfinite(config_.minimumRealtimeSpeed) ||
        config_.minimumRealtimeSpeed < 1.0 ||
        config_.minimumRealtimeSpeed > 10.0) {
        config_.minimumRealtimeSpeed = 1.25;
    }
}

MediaTranscodePresetMode MediaTranscodePolicy::presetModeFromString(
    const std::string& value,
    bool& valid)
{
    valid = true;
    if (value == "auto") return MediaTranscodePresetMode::Auto;
    if (value == "superfast") return MediaTranscodePresetMode::Superfast;
    if (value == "veryfast") return MediaTranscodePresetMode::Veryfast;
    if (value == "faster") return MediaTranscodePresetMode::Faster;
    if (value == "fast") return MediaTranscodePresetMode::Fast;
    valid = false;
    return MediaTranscodePresetMode::Auto;
}

MediaVideoEncoderMode MediaTranscodePolicy::videoEncoderModeFromString(
    const std::string& value,
    bool& valid)
{
    valid = true;
    if (value == "auto") return MediaVideoEncoderMode::Auto;
    if (value == "software") return MediaVideoEncoderMode::Software;
    if (value == "vaapi") return MediaVideoEncoderMode::Vaapi;
    valid = false;
    return MediaVideoEncoderMode::Auto;
}

const char* MediaTranscodePolicy::videoEncoderModeName(MediaVideoEncoderMode mode)
{
    switch (mode) {
    case MediaVideoEncoderMode::Auto: return "auto";
    case MediaVideoEncoderMode::Software: return "software";
    case MediaVideoEncoderMode::Vaapi: return "vaapi";
    }
    return "auto";
}

MediaTranscodePolicy MediaTranscodePolicy::fromEnvironment(
    std::optional<MediaVideoEncoderMode> managedMode,
    std::optional<bool> vaapiAvailable)
{
    MediaTranscodePolicyConfig config;

    if (managedMode.has_value()) {
        config.videoEncoderMode = managedMode.value();
    }
    else if (const char* raw = std::getenv("VDR_SUITE_MEDIA_VIDEO_ENCODER")) {
        bool valid = false;
        const MediaVideoEncoderMode mode = videoEncoderModeFromString(raw, valid);
        if (valid) config.videoEncoderMode = mode;
    }

    if (const char* raw = std::getenv("VDR_SUITE_MEDIA_X264_PRESET")) {
        bool valid = false;
        const MediaTranscodePresetMode mode = presetModeFromString(raw, valid);
        if (valid) config.globalPresetMode = mode;
    }

    config.standardPresetMode = optionalModeFromEnvironment(
        "VDR_SUITE_MEDIA_X264_STANDARD_PRESET");
    config.deinterlacePresetMode = optionalModeFromEnvironment(
        "VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET");
    config.uhdSourcePresetMode = optionalModeFromEnvironment(
        "VDR_SUITE_MEDIA_X264_UHD_PRESET");

    bool vaapiDeviceOverridden = false;
    if (const char* raw = std::getenv("VDR_SUITE_MEDIA_VAAPI_DEVICE")) {
        if (*raw != '\0' && validVaapiDevice(raw)) {
            config.vaapiDevice = raw;
            vaapiDeviceOverridden = true;
        }
    }

    std::string profilePath = DefaultPerformanceProfilePath;
    if (const char* raw = std::getenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE")) {
        if (*raw != '\0') profilePath = raw;
    }

    LoadedPerformanceProfile performance = loadPerformanceProfile(profilePath);
    config.calibrationProfilePresent = performance.present;
    config.calibrationProfileValid = performance.valid;
    if (!vaapiDeviceOverridden && !performance.vaapiDevice.empty()) {
        config.vaapiDevice = performance.vaapiDevice;
    }

    if (vaapiAvailable.has_value()) {
        config.vaapiAvailable = vaapiAvailable.value();
    }
    else {
        std::error_code error;
        config.vaapiAvailable = validVaapiDevice(config.vaapiDevice) &&
            std::filesystem::exists(config.vaapiDevice, error) && !error;
    }

    return MediaTranscodePolicy(
        std::move(config),
        std::move(performance.software),
        std::move(performance.hardware));
}

MediaTranscodePresetMode MediaTranscodePolicy::modeFor(
    MediaTranscodeWorkload workload) const
{
    const std::optional<MediaTranscodePresetMode>* specific = nullptr;
    switch (workload) {
    case MediaTranscodeWorkload::Standard:
        specific = &config_.standardPresetMode;
        break;
    case MediaTranscodeWorkload::Deinterlace:
        specific = &config_.deinterlacePresetMode;
        break;
    case MediaTranscodeWorkload::UhdSource:
        specific = &config_.uhdSourcePresetMode;
        break;
    case MediaTranscodeWorkload::None:
        break;
    }
    if (specific != nullptr && specific->has_value()) {
        return specific->value();
    }
    return config_.globalPresetMode;
}

std::optional<MediaSoftwareEncoderPreset> MediaTranscodePolicy::selectMeasuredPreset(
    MediaTranscodeWorkload workload) const
{
    if (!config_.calibrationProfileValid) return std::nullopt;
    const auto workloadSamples = samples_.find(workload);
    if (workloadSamples == samples_.end()) return std::nullopt;

    static constexpr std::array<MediaSoftwareEncoderPreset, 4> QualityOrder = {
        MediaSoftwareEncoderPreset::Fast,
        MediaSoftwareEncoderPreset::Faster,
        MediaSoftwareEncoderPreset::Veryfast,
        MediaSoftwareEncoderPreset::Superfast
    };
    for (MediaSoftwareEncoderPreset preset : QualityOrder) {
        const auto sample = workloadSamples->second.find(preset);
        if (sample != workloadSamples->second.end() &&
            sample->second >= config_.minimumRealtimeSpeed) {
            return preset;
        }
    }
    return std::nullopt;
}

MediaSoftwareEncoderPreset MediaTranscodePolicy::selectPreset(
    MediaTranscodeWorkload workload) const
{
    const MediaTranscodePresetMode mode = modeFor(workload);
    if (mode != MediaTranscodePresetMode::Auto) {
        return presetForMode(mode);
    }

    const std::optional<MediaSoftwareEncoderPreset> measured =
        selectMeasuredPreset(workload);
    if (measured.has_value()) return measured.value();

    return conservativeFallback(workload);
}

std::optional<double> MediaTranscodePolicy::hardwareSpeed(
    MediaTranscodeWorkload workload,
    MediaVideoEncoderBackend backend) const
{
    if (!config_.calibrationProfileValid) return std::nullopt;
    const auto workloadSamples = hardwareSamples_.find(workload);
    if (workloadSamples == hardwareSamples_.end()) return std::nullopt;
    const auto sample = workloadSamples->second.find(backend);
    if (sample == workloadSamples->second.end()) return std::nullopt;
    return sample->second;
}

bool MediaTranscodePolicy::vaapiTransformationSupported(
    const MediaPresentationProfile& profile) const
{
    return profile.targetVideoCodec == MediaCodec::H264 &&
        !profile.deinterlaceVideo &&
        profile.targetVideoWidth > 0 &&
        profile.targetVideoHeight > 0;
}

MediaTranscodePolicyDiagnostics MediaTranscodePolicy::diagnostics() const
{
    MediaTranscodePolicyDiagnostics result;
    result.videoEncoderMode = config_.videoEncoderMode;
    result.calibrationProfilePresent = config_.calibrationProfilePresent;
    result.calibrationProfileValid = config_.calibrationProfileValid;
    result.minimumRealtimeSpeed = config_.minimumRealtimeSpeed;
    result.softwareCalibrated = config_.calibrationProfileValid &&
        anySoftwareCalibration(samples_);
    result.softwareSuitable = config_.calibrationProfileValid &&
        anySoftwareSuitable(samples_, config_.minimumRealtimeSpeed);
    result.vaapiAvailable = config_.vaapiAvailable &&
        validVaapiDevice(config_.vaapiDevice);
    result.vaapiCalibrated = config_.calibrationProfileValid &&
        anyVaapiCalibration(hardwareSamples_);
    result.vaapiSuitable = result.vaapiAvailable &&
        config_.calibrationProfileValid &&
        anyVaapiSuitable(hardwareSamples_, config_.minimumRealtimeSpeed);
    result.forcedVaapiBelowThreshold =
        config_.videoEncoderMode == MediaVideoEncoderMode::Vaapi &&
        result.vaapiAvailable && result.vaapiCalibrated &&
        anyVaapiBelowThreshold(hardwareSamples_, config_.minimumRealtimeSpeed);

    if (!result.vaapiAvailable) {
        result.vaapiReason = "VAAPI device is not available on the execution host";
    }
    else if (!result.vaapiCalibrated) {
        result.vaapiReason = "VAAPI has no valid calibration evidence";
    }
    else if (!result.vaapiSuitable) {
        result.vaapiReason = "VAAPI calibration is below the automatic real-time threshold";
    }
    else {
        result.vaapiReason = "VAAPI is eligible for calibrated automatic workloads";
    }
    return result;
}

MediaPresentationProfile MediaTranscodePolicy::apply(
    const MediaPresentationProfile& profile) const
{
    MediaPresentationProfile result = profile;
    if (result.videoEncoderPolicyResolved ||
        !result.available || result.videoAction != MediaTrackAction::Transcode) {
        return result;
    }

    if (result.targetVideoCodec != MediaCodec::H264) {
        result.available = false;
        result.reason = "requested video transcode codec has no implemented encoder backend";
        return result;
    }

    MediaTranscodeWorkload workload = result.videoTranscodeWorkload;
    if (workload == MediaTranscodeWorkload::None) {
        workload = result.deinterlaceVideo
            ? MediaTranscodeWorkload::Deinterlace
            : MediaTranscodeWorkload::Standard;
        result.videoTranscodeWorkload = workload;
    }

    if (config_.videoEncoderMode == MediaVideoEncoderMode::Software) {
        result.videoEncoderBackend = MediaVideoEncoderBackend::SoftwareX264;
        result.videoEncoderPreset = selectPreset(workload);
        result.videoHardwareDevice.clear();
        result.videoEncoderPolicyResolved = true;
        return result;
    }

    const bool vaapiSupported = vaapiTransformationSupported(result);
    const bool vaapiAvailable = config_.vaapiAvailable &&
        validVaapiDevice(config_.vaapiDevice);

    if (config_.videoEncoderMode == MediaVideoEncoderMode::Vaapi) {
        if (!vaapiSupported) {
            result.available = false;
            result.reason = "forced VAAPI does not support the requested video transformation";
            return result;
        }
        if (!vaapiAvailable) {
            result.available = false;
            result.reason = "forced VAAPI is unavailable on the execution host";
            return result;
        }
        result.videoEncoderBackend = MediaVideoEncoderBackend::Vaapi;
        result.videoHardwareDevice = config_.vaapiDevice;
        result.videoEncoderPolicyResolved = true;
        return result;
    }

    if (vaapiSupported && vaapiAvailable) {
        const std::optional<double> measured = hardwareSpeed(
            workload, MediaVideoEncoderBackend::Vaapi);
        if (measured.has_value() &&
            measured.value() >= config_.minimumRealtimeSpeed) {
            result.videoEncoderBackend = MediaVideoEncoderBackend::Vaapi;
            result.videoHardwareDevice = config_.vaapiDevice;
            result.videoEncoderPolicyResolved = true;
            return result;
        }
    }

    const std::optional<MediaSoftwareEncoderPreset> measured =
        selectMeasuredPreset(workload);
    if (measured.has_value()) {
        result.videoEncoderBackend = MediaVideoEncoderBackend::SoftwareX264;
        result.videoEncoderPreset = measured.value();
        result.videoHardwareDevice.clear();
        result.videoEncoderPolicyResolved = true;
        return result;
    }

    result.available = false;
    result.reason =
        "no calibrated video encoder backend reaches minimum real-time speed";
    return result;
}
