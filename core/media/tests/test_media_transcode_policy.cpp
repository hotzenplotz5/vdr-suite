#include "MediaTranscodePolicy.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{

MediaPresentationProfile transcodeProfile(MediaTranscodeWorkload workload)
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = MediaContainer::Fmp4;
    profile.videoAction = MediaTrackAction::Transcode;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetVideoWidth = 1920;
    profile.targetVideoHeight = 1080;
    profile.videoTranscodeWorkload = workload;
    return profile;
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
    ::unsetenv("VDR_SUITE_MEDIA_X264_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_STANDARD_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_X264_UHD_PRESET");
    ::unsetenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE");
}

} // namespace

int main()
{
    clearPolicyEnvironment();

    {
        MediaTranscodePolicy policy;
        assert(policy.selectPreset(MediaTranscodeWorkload::Standard) ==
            MediaSoftwareEncoderPreset::Veryfast);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Superfast);
        assert(policy.selectPreset(MediaTranscodeWorkload::UhdSource) ==
            MediaSoftwareEncoderPreset::Veryfast);
    }

    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Veryfast] = 0.992;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Superfast] = 1.54;

        MediaTranscodePolicy policy(MediaTranscodePolicyConfig{}, samples);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Superfast);
    }

    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Fast] = 1.31;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Faster] = 1.55;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Veryfast] = 1.85;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Superfast] = 2.2;

        MediaTranscodePolicy policy(MediaTranscodePolicyConfig{}, samples);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Fast);
    }

    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::Standard][
            MediaSoftwareEncoderPreset::Fast] = 1.10;
        samples[MediaTranscodeWorkload::Standard][
            MediaSoftwareEncoderPreset::Faster] = 1.30;
        samples[MediaTranscodeWorkload::Standard][
            MediaSoftwareEncoderPreset::Veryfast] = 1.60;

        MediaTranscodePolicy policy(MediaTranscodePolicyConfig{}, samples);
        assert(policy.selectPreset(MediaTranscodeWorkload::Standard) ==
            MediaSoftwareEncoderPreset::Faster);
    }

    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::UhdSource][
            MediaSoftwareEncoderPreset::Superfast] = 0.94;
        samples[MediaTranscodeWorkload::UhdSource][
            MediaSoftwareEncoderPreset::Veryfast] = 0.86;
        MediaTranscodePolicy policy(MediaTranscodePolicyConfig{}, samples);
        assert(policy.selectPreset(MediaTranscodeWorkload::UhdSource) ==
            MediaSoftwareEncoderPreset::Veryfast);
    }

    {
        MediaTranscodePolicyConfig config;
        config.globalPresetMode = MediaTranscodePresetMode::Fast;
        MediaTranscodePolicy policy(config);
        assert(policy.selectPreset(MediaTranscodeWorkload::Standard) ==
            MediaSoftwareEncoderPreset::Fast);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Fast);
    }

    {
        MediaTranscodePolicyConfig config;
        config.globalPresetMode = MediaTranscodePresetMode::Veryfast;
        config.deinterlacePresetMode = MediaTranscodePresetMode::Superfast;
        MediaTranscodePolicy policy(config);
        assert(policy.selectPreset(MediaTranscodeWorkload::Standard) ==
            MediaSoftwareEncoderPreset::Veryfast);
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Superfast);
    }

    {
        MediaTranscodePerformanceSamples samples;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Veryfast] = 0.992;
        samples[MediaTranscodeWorkload::Deinterlace][
            MediaSoftwareEncoderPreset::Superfast] = 1.54;
        MediaTranscodePolicy policy(MediaTranscodePolicyConfig{}, samples);

        MediaPresentationProfile profile =
            transcodeProfile(MediaTranscodeWorkload::Deinterlace);
        profile.deinterlaceVideo = true;
        const MediaPresentationProfile resolved = policy.apply(profile);
        assert(resolved.videoEncoderBackend ==
            MediaVideoEncoderBackend::SoftwareX264);
        assert(resolved.videoEncoderPreset ==
            MediaSoftwareEncoderPreset::Superfast);
    }

    {
        bool valid = false;
        assert(MediaTranscodePolicy::presetModeFromString("auto", valid) ==
            MediaTranscodePresetMode::Auto && valid);
        assert(MediaTranscodePolicy::presetModeFromString("faster", valid) ==
            MediaTranscodePresetMode::Faster && valid);
        (void)MediaTranscodePolicy::presetModeFromString("nonsense", valid);
        assert(!valid);
    }

    for (const std::string version : {"1", "2"}) {
        const auto path = profilePath("v" + version + ".conf");
        writeProfile(
            path,
            "version=" + version + "\n"
            "deinterlace.superfast=1.950\n"
            "deinterlace.veryfast=1.540\n");
        ::setenv("VDR_SUITE_MEDIA_X264_PRESET", "auto", 1);
        ::setenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE", path.c_str(), 1);

        const MediaTranscodePolicy policy = MediaTranscodePolicy::fromEnvironment();
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Superfast);

        std::filesystem::remove(path);
        clearPolicyEnvironment();
    }

    {
        const auto path = profilePath("v3.conf");
        writeProfile(
            path,
            "version=3\n"
            "deinterlace.superfast=1.950\n"
            "deinterlace.veryfast=1.540\n");
        ::setenv("VDR_SUITE_MEDIA_X264_PRESET", "auto", 1);
        ::setenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE", path.c_str(), 1);

        const MediaTranscodePolicy policy = MediaTranscodePolicy::fromEnvironment();
        assert(policy.selectPreset(MediaTranscodeWorkload::Deinterlace) ==
            MediaSoftwareEncoderPreset::Veryfast);

        std::filesystem::remove(path);
        clearPolicyEnvironment();
    }

    return 0;
}
