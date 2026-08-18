#include "MediaTranscodePolicy.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{

constexpr const char* DefaultPerformanceProfilePath =
    "/var/lib/vdr-suite/media-transcode-performance.conf";

std::string trim(const std::string& value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
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
    if (workload == MediaTranscodeWorkload::Deinterlace) {
        return MediaSoftwareEncoderPreset::Superfast;
    }
    return MediaSoftwareEncoderPreset::Veryfast;
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

MediaTranscodePerformanceSamples loadPerformanceSamples(
    const std::string& path)
{
    MediaTranscodePerformanceSamples samples;
    std::ifstream input(path);
    if (!input.is_open()) return samples;

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') continue;

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos || equals == 0) continue;

        const std::string key = trim(line.substr(0, equals));
        const std::string value = trim(line.substr(equals + 1));
        const std::size_t dot = key.find('.');
        if (dot == std::string::npos || dot == 0 || dot + 1 >= key.size()) {
            continue;
        }

        MediaTranscodeWorkload workload = MediaTranscodeWorkload::None;
        MediaSoftwareEncoderPreset preset = MediaSoftwareEncoderPreset::Veryfast;
        double speed = 0.0;
        if (!workloadFromString(key.substr(0, dot), workload) ||
            !presetFromString(key.substr(dot + 1), preset) ||
            !positiveFiniteDouble(value, speed)) {
            continue;
        }
        samples[workload][preset] = speed;
    }
    return samples;
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

} // namespace

MediaTranscodePolicy::MediaTranscodePolicy(
    MediaTranscodePolicyConfig config,
    MediaTranscodePerformanceSamples samples)
    : config_(std::move(config)),
      samples_(std::move(samples))
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

MediaTranscodePolicy MediaTranscodePolicy::fromEnvironment()
{
    MediaTranscodePolicyConfig config;

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

    std::string profilePath = DefaultPerformanceProfilePath;
    if (const char* raw = std::getenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE")) {
        if (*raw != '\0') profilePath = raw;
    }

    return MediaTranscodePolicy(
        std::move(config),
        loadPerformanceSamples(profilePath));
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

MediaSoftwareEncoderPreset MediaTranscodePolicy::selectPreset(
    MediaTranscodeWorkload workload) const
{
    const MediaTranscodePresetMode mode = modeFor(workload);
    if (mode != MediaTranscodePresetMode::Auto) {
        return presetForMode(mode);
    }

    const auto workloadSamples = samples_.find(workload);
    if (workloadSamples != samples_.end()) {
        // Prefer the slowest/highest-compression x264 preset that still has
        // enough measured real-time headroom for HLS muxing and system load.
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
    }

    return conservativeFallback(workload);
}

MediaPresentationProfile MediaTranscodePolicy::apply(
    const MediaPresentationProfile& profile) const
{
    MediaPresentationProfile result = profile;
    if (!result.available ||
        result.videoAction != MediaTrackAction::Transcode ||
        result.targetVideoCodec != MediaCodec::H264) {
        return result;
    }

    MediaTranscodeWorkload workload = result.videoTranscodeWorkload;
    if (workload == MediaTranscodeWorkload::None) {
        workload = result.deinterlaceVideo
            ? MediaTranscodeWorkload::Deinterlace
            : MediaTranscodeWorkload::Standard;
        result.videoTranscodeWorkload = workload;
    }

    // The policy boundary owns encoder selection. Software x264 is the only
    // implemented backend today; VAAPI/QSV/NVENC can be introduced here later
    // without teaching the presentation selector or browser about server HW.
    result.videoEncoderBackend = MediaVideoEncoderBackend::SoftwareX264;
    result.videoEncoderPreset = selectPreset(workload);
    return result;
}
