#include "MediaTranscodePolicy.h"

#include <cassert>

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

} // namespace

int main()
{
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

    return 0;
}
