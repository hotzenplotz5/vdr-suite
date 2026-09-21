#include "RecordingHlsResumeProfile.h"

#include "MediaTranscodePolicy.h"

#include <cassert>

namespace
{

MediaPresentationProfile copyProfile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "hls-fmp4";
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Remux;
    profile.videoAction = MediaTrackAction::Copy;
    profile.audioAction = MediaTrackAction::Copy;
    profile.sourceVideoStreamIndex = 0;
    profile.sourceAudioStreamIndex = 0;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetAudioCodec = MediaCodec::Aac;
    profile.targetVideoWidth = 1920;
    profile.targetVideoHeight = 1080;
    profile.targetAudioChannels = 2;
    return profile;
}

} // namespace

int main()
{
    const MediaPresentationProfile copy = copyProfile();

    const MediaPresentationProfile ordinary =
        RecordingHlsResumeProfile::prepare(copy, 0);
    assert(ordinary.available);
    assert(ordinary.videoAction == MediaTrackAction::Copy);
    assert(ordinary.audioAction == MediaTrackAction::Copy);
    assert(ordinary.adaptationClass == MediaAdaptationClass::Remux);

    const MediaPresentationProfile resumed =
        RecordingHlsResumeProfile::prepare(copy, 2494);
    assert(resumed.available);
    assert(resumed.videoAction == MediaTrackAction::Transcode);
    assert(resumed.audioAction == MediaTrackAction::Transcode);
    assert(resumed.adaptationClass == MediaAdaptationClass::Transcode);
    assert(resumed.videoTranscodeWorkload == MediaTranscodeWorkload::Standard);
    assert(resumed.videoEncoderPolicyResolved == false);
    assert(resumed.targetVideoCodec == MediaCodec::H264);
    assert(resumed.targetAudioCodec == MediaCodec::Aac);
    assert(resumed.targetVideoWidth == 1920);
    assert(resumed.targetVideoHeight == 1080);
    assert(resumed.targetAudioChannels == 2);
    assert(resumed.sourceVideoStreamIndex == 0);
    assert(resumed.sourceAudioStreamIndex == 0);

    // createSession() resolves the backend-scoped policy before provisionHlsAt().
    // The runtime owns a separate environment-derived policy and applies it again.
    // A resolved managed choice must therefore be immutable across that second pass.
    MediaTranscodePolicyConfig managedConfig;
    managedConfig.videoEncoderMode = MediaVideoEncoderMode::Software;
    managedConfig.globalPresetMode = MediaTranscodePresetMode::Fast;
    const MediaPresentationProfile managedResolved =
        MediaTranscodePolicy(managedConfig).apply(resumed);
    assert(managedResolved.available);
    assert(managedResolved.videoEncoderPolicyResolved);
    assert(managedResolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
    assert(managedResolved.videoEncoderPreset == MediaSoftwareEncoderPreset::Fast);
    assert(managedResolved.videoHardwareDevice.empty());

    MediaTranscodePolicyConfig conflictingRuntimeConfig;
    conflictingRuntimeConfig.videoEncoderMode = MediaVideoEncoderMode::Vaapi;
    conflictingRuntimeConfig.vaapiAvailable = true;
    conflictingRuntimeConfig.vaapiDevice = "/dev/dri/renderD128";
    const MediaPresentationProfile runtimeResolved =
        MediaTranscodePolicy(conflictingRuntimeConfig).apply(managedResolved);
    assert(runtimeResolved.available);
    assert(runtimeResolved.videoEncoderPolicyResolved);
    assert(runtimeResolved.videoEncoderBackend == MediaVideoEncoderBackend::SoftwareX264);
    assert(runtimeResolved.videoEncoderPreset == MediaSoftwareEncoderPreset::Fast);
    assert(runtimeResolved.videoHardwareDevice.empty());
    assert(runtimeResolved.videoAction == MediaTrackAction::Transcode);
    assert(runtimeResolved.audioAction == MediaTrackAction::Transcode);

    MediaPresentationProfile mixed = copyProfile();
    mixed.adaptationClass = MediaAdaptationClass::Transcode;
    mixed.videoAction = MediaTrackAction::Transcode;
    mixed.videoTranscodeWorkload = MediaTranscodeWorkload::Standard;
    const MediaPresentationProfile mixedResume =
        RecordingHlsResumeProfile::prepare(mixed, 1200);
    assert(mixedResume.available);
    assert(mixedResume.videoAction == MediaTrackAction::Transcode);
    assert(mixedResume.audioAction == MediaTrackAction::Transcode);

    MediaPresentationProfile audioOnly = copyProfile();
    audioOnly.videoAction = MediaTrackAction::Omit;
    audioOnly.sourceVideoStreamIndex = -1;
    audioOnly.targetVideoCodec = MediaCodec::None;
    audioOnly.targetVideoWidth = 0;
    audioOnly.targetVideoHeight = 0;
    const MediaPresentationProfile audioOnlyResume =
        RecordingHlsResumeProfile::prepare(audioOnly, 600);
    assert(audioOnlyResume.available);
    assert(audioOnlyResume.videoAction == MediaTrackAction::Omit);
    assert(audioOnlyResume.audioAction == MediaTrackAction::Copy);

    MediaPresentationProfile unsupportedVideo = copyProfile();
    unsupportedVideo.targetVideoCodec = MediaCodec::H265;
    const MediaPresentationProfile unsupportedVideoResume =
        RecordingHlsResumeProfile::prepare(unsupportedVideo, 600);
    assert(!unsupportedVideoResume.available);
    assert(unsupportedVideoResume.reason ==
        "exact HLS video resume requires implemented H.264 transcode");

    MediaPresentationProfile unsupportedAudio = copyProfile();
    unsupportedAudio.targetAudioCodec = MediaCodec::Ac3;
    const MediaPresentationProfile unsupportedAudioResume =
        RecordingHlsResumeProfile::prepare(unsupportedAudio, 600);
    assert(!unsupportedAudioResume.available);
    assert(unsupportedAudioResume.reason ==
        "exact HLS video resume requires implemented AAC audio transcode");

    return 0;
}
