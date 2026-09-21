#include "MediaTranscodePolicy.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{

MediaPresentationProfile transcodeProfile(
    MediaTranscodeWorkload workload,
    MediaDeliveryProtocol protocol = MediaDeliveryProtocol::Hls)
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = protocol == MediaDeliveryProtocol::Hls
        ? "hls-fmp4"
        : "progressive-fmp4";
    profile.protocol = protocol;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Transcode;
    profile.videoAction = MediaTrackAction::Transcode;
    profile.audioAction = MediaTrackAction::Copy;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetVideoWidth = 1920;
    profile.targetVideoHeight = 1080;
    profile.videoTranscodeWorkload = workload;
    return profile;
}

MediaTranscodePolicyConfig calibratedConfig(
    MediaVideoEncoderMode mode = MediaVideoEncoderMode::Auto)
{
    MediaTranscodePolicyConfig config;
    config.videoEncoderMode = mode;
    config.calibrationProfilePresent = true;
    config.calibrationProfileValid = true;
    return config;
}

std::filesystem::path profilePath(const std::string& suffix)
{
    return std::filesystem::temp_directory_path() /
        ("vdr-suite-media-policy-" + std::to_string(::getpid()) + "-" + suffix);
}

void writeProfile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream output(path, std::ios::trunc);
    assert(output.is_open());
    output << content;
    output.close();
    assert(output.good());
}

void clearPolicyEnvironment()
{
    ::unsetenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
    ::unsetenv("VDR_SUITE_MEDIA_X264_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_STANDARD_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_UHD_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_VAAPI_DEVICE");
    ::unsetenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE");
}

} // namespace

int main()
{
    clearPolicyEnvironment();

    // Copy/Omit and audio-only adaptation never consult video encoder mode.
    for (MediaVideoEncoderMode mode : {
            MediaVideoEncoderMode::Auto,
            MediaVideoEncoderMode::Software,
            MediaVideoEncoderMode::Vaapi}) {
        MediaTranscodePolicyConfig config;
        config.videoEncoderMode = mode;
        MediaTranscodePolicy policy(config);

        MediaPresentationProfile copy = transcodeProfile(MediaTranscodeWorkload::Standard);
        copy.videoAction = MediaTrackAction::Copy;
        copy.audioAction = MediaTrackAction::Transcode;
        copy.targetAudioCodec = MediaCodec::Aac;
        const auto copyResolved = policy.apply(copy);
        assert(copyResolved.available);
        assert(copyResolved.videoAction == MediaTrackAction::Copy);
        assert(copyResolved.videoEncoderBackend == copy.videoEncoderBackend);

        MediaPresentationProfile omit = copy;
        omit.videoAction = MediaTrackAction::Omit;
        const auto omitResolved = policy.apply(omit);
        assert(omitResolved.available);
        assert(omitResolved.videoAction == MediaTrackAction::Omit);
    }

    // Auto prefers eligible, measured VAAPI.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        config.vaapiAvailable = true;
        MediaTranscodePerformanceSamples software;
        software[MediaTranscodeWorkload::UhdSource][MediaSoftwareEncoderPreset::Superfast] = 1.60;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::UhdSource][MediaVideoEncoderBackend::Vaapi] = 3.825;
        MediaTranscodePolicy policy(config, software, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::UhdSource));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi);
    }

    // Auto rejects VAAPI for an unsupported transform and falls back to measured x264.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        config.vaapiAvailable = true;
        MediaTranscodePerformanceSamples software;
        software[MediaTranscodeWorkload::Deinterlace][MediaSoftwareEncoderPreset::Superfast] = 1.54;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Deinterlace][MediaVideoEncoderBackend::Vaapi] = 3.20;
        MediaTranscodePolicy policy(config, software, hardware);
        MediaPresentationProfile profile = transcodeProfile(MediaTranscodeWorkload::Deinterlace);
        profile.deinterlaceVideo = true;
        const auto resolved = policy.apply(profile);
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
        assert(resolved.videoEncoderPreset == MediaSoftwareEncoderPreset::Superfast);
    }

    // Auto deterministically falls back to suitable measured software.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        config.vaapiAvailable = true;
        MediaTranscodePerformanceSamples software;
        software[MediaTranscodeWorkload::Standard][MediaSoftwareEncoderPreset::Faster] = 1.55;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Vaapi] = 1.10;
        MediaTranscodePolicy policy(config, software, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
        assert(resolved.videoEncoderPreset == MediaSoftwareEncoderPreset::Faster);
    }

    // Auto fails closed when no calibrated backend meets the real-time contract.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        config.vaapiAvailable = true;
        MediaTranscodePerformanceSamples software;
        software[MediaTranscodeWorkload::Standard][MediaSoftwareEncoderPreset::Superfast] = 1.20;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Vaapi] = 1.10;
        MediaTranscodePolicy policy(config, software, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(!resolved.available);
        assert(resolved.reason ==
            "no calibrated video encoder backend reaches minimum real-time speed");
    }

    // Missing/invalid calibration also fails auto closed.
    {
        MediaTranscodePolicy policy;
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(!resolved.available);
    }

    // Forced software always selects x264 and never VAAPI.
    {
        MediaTranscodePolicyConfig config;
        config.videoEncoderMode = MediaVideoEncoderMode::Software;
        config.vaapiAvailable = true;
        config.globalPresetMode = MediaTranscodePresetMode::Fast;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Vaapi] = 5.0;
        MediaTranscodePolicy policy(config, {}, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
        assert(resolved.videoEncoderPreset == MediaSoftwareEncoderPreset::Fast);
        assert(resolved.videoHardwareDevice.empty());
    }

    // Forced VAAPI never silently falls back to x264 and may override only the threshold.
    {
        MediaTranscodePolicyConfig config = calibratedConfig(MediaVideoEncoderMode::Vaapi);
        config.vaapiAvailable = true;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Vaapi] = 0.90;
        MediaTranscodePolicy policy(config, {}, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi);
        const auto diagnostics = policy.diagnostics();
        assert(diagnostics.forcedVaapiBelowThreshold);
        assert(!diagnostics.vaapiSuitable);
    }

    // Forced VAAPI respects hard transform capability and fails closed.
    {
        MediaTranscodePolicyConfig config;
        config.videoEncoderMode = MediaVideoEncoderMode::Vaapi;
        config.vaapiAvailable = true;
        MediaTranscodePolicy policy(config);
        MediaPresentationProfile profile = transcodeProfile(MediaTranscodeWorkload::Deinterlace);
        profile.deinterlaceVideo = true;
        const auto resolved = policy.apply(profile);
        assert(!resolved.available);
        assert(resolved.reason.find("forced VAAPI") != std::string::npos);
    }

    // HLS/progressive-fMP4 resolve the same encoder backend for equivalent work.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        config.vaapiAvailable = true;
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::UhdSource][MediaVideoEncoderBackend::Vaapi] = 2.0;
        MediaTranscodePolicy policy(config, {}, hardware);
        const auto hls = policy.apply(transcodeProfile(
            MediaTranscodeWorkload::UhdSource, MediaDeliveryProtocol::Hls));
        const auto progressive = policy.apply(transcodeProfile(
            MediaTranscodeWorkload::UhdSource, MediaDeliveryProtocol::Progressive));
        assert(hls.available && progressive.available);
        assert(hls.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi);
        assert(progressive.videoEncoderBackend == hls.videoEncoderBackend);
    }

    // QSV/NVENC calibration entries remain modeled but cannot be selected.
    {
        MediaTranscodePolicyConfig config = calibratedConfig();
        MediaHardwareTranscodePerformanceSamples hardware;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Qsv] = 9.0;
        hardware[MediaTranscodeWorkload::Standard][MediaVideoEncoderBackend::Nvenc] = 9.0;
        MediaTranscodePerformanceSamples software;
        software[MediaTranscodeWorkload::Standard][MediaSoftwareEncoderPreset::Veryfast] = 1.50;
        MediaTranscodePolicy policy(config, software, hardware);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Standard));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
    }

    // Preset policy remains orthogonal to backend mode.
    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::Deinterlace][MediaSoftwareEncoderPreset::Veryfast] = 0.992;
        samples[MediaTranscodeWorkload::Deinterlace][MediaSoftwareEncoderPreset::Superfast] = 1.54;
        MediaTranscodePolicyConfig config = calibratedConfig(MediaVideoEncoderMode::Software);
        MediaTranscodePolicy policy(config, samples);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Superfast);
    }

    {
        bool valid = false;
        assert(MediaTranscodePolicy::presetModeFromString("auto", valid) ==
            MediaTranscodePresetMode::Auto && valid);
        assert(MediaTranscodePolicy::videoEncoderModeFromString("software", valid) ==
            MediaVideoEncoderMode::Software && valid);
        assert(MediaTranscodePolicy::videoEncoderModeFromString("vaapi", valid) ==
            MediaVideoEncoderMode::Vaapi && valid);
        (void)MediaTranscodePolicy::videoEncoderModeFromString("qsv", valid);
        assert(!valid);
        (void)MediaTranscodePolicy::videoEncoderModeFromString("nvenc", valid);
        assert(!valid);
    }

    // Managed mode overrides environment; environment overrides built-in auto.
    {
        ::setenv("VDR_SUITE_MEDIA_VIDEO_ENCODER", "software", 1);
        MediaTranscodePolicy environment = MediaTranscodePolicy::fromEnvironment(
            std::nullopt, false);
        assert(environment.videoEncoderMode() == MediaVideoEncoderMode::Software);
        MediaTranscodePolicy managed = MediaTranscodePolicy::fromEnvironment(
            MediaVideoEncoderMode::Vaapi, true);
        assert(managed.videoEncoderMode() == MediaVideoEncoderMode::Vaapi);
        clearPolicyEnvironment();
    }

    for (const std::string version : {"1", "2"}) {
        const auto path = profilePath("v" + version + ".conf");
        writeProfile(path,
            "version=" + version + "\n"
            "deinterlace.superfast=1.950\n"
            "deinterlace.veryfast=1.540\n");
        ::setenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE", path.c_str(), 1);
        const MediaTranscodePolicy policy = MediaTranscodePolicy::fromEnvironment(
            MediaVideoEncoderMode::Auto, false);
        const auto diagnostics = policy.diagnostics();
        assert(diagnostics.calibrationProfilePresent);
        assert(!diagnostics.calibrationProfileValid);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::Deinterlace));
        assert(!resolved.available);
        std::filesystem::remove(path);
        clearPolicyEnvironment();
    }

    {
        const auto path = profilePath("v4.conf");
        writeProfile(path,
            "version=4\n"
            "standard.faster=1.450\n"
            "uhd-source.superfast=0.468\n"
            "uhd-source.vaapi=3.825\n"
            "vaapi.device=/dev/dri/renderD129\n");
        ::setenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE", path.c_str(), 1);
        const MediaTranscodePolicy policy = MediaTranscodePolicy::fromEnvironment(
            MediaVideoEncoderMode::Auto, true);
        const auto diagnostics = policy.diagnostics();
        assert(diagnostics.calibrationProfilePresent);
        assert(diagnostics.calibrationProfileValid);
        assert(diagnostics.softwareCalibrated);
        assert(diagnostics.softwareSuitable);
        assert(diagnostics.vaapiCalibrated);
        assert(diagnostics.vaapiSuitable);
        const auto resolved = policy.apply(transcodeProfile(MediaTranscodeWorkload::UhdSource));
        assert(resolved.available);
        assert(resolved.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi);
        assert(resolved.videoHardwareDevice == "/dev/dri/renderD129");
        std::filesystem::remove(path);
        clearPolicyEnvironment();
    }

    return 0;
}
