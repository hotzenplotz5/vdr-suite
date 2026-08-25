#include "RecordingHlsResumeProfile.h"

namespace
{

MediaTranscodeWorkload workloadFor(const MediaPresentationProfile& profile)
{
    if (profile.deinterlaceVideo) {
        return MediaTranscodeWorkload::Deinterlace;
    }
    if (profile.targetVideoWidth > 1920 || profile.targetVideoHeight > 1080) {
        return MediaTranscodeWorkload::UhdSource;
    }
    return MediaTranscodeWorkload::Standard;
}

} // namespace

MediaPresentationProfile RecordingHlsResumeProfile::prepare(
    const MediaPresentationProfile& profile,
    int startPositionSeconds)
{
    MediaPresentationProfile result = profile;
    if (startPositionSeconds <= 0 || !result.available ||
        result.protocol != MediaDeliveryProtocol::Hls ||
        result.videoAction == MediaTrackAction::Omit) {
        return result;
    }

    if (result.videoAction == MediaTrackAction::Copy) {
        if (result.targetVideoCodec != MediaCodec::H264) {
            result.available = false;
            result.reason =
                "exact HLS video resume requires implemented H.264 transcode";
            return result;
        }
        result.videoAction = MediaTrackAction::Transcode;
        result.videoTranscodeWorkload = workloadFor(result);
        result.videoEncoderPolicyResolved = false;
        result.videoHardwareDevice.clear();
        result.adaptationClass = MediaAdaptationClass::Transcode;
    }

    if (result.audioAction == MediaTrackAction::Copy) {
        if (result.targetAudioCodec != MediaCodec::Aac) {
            result.available = false;
            result.reason =
                "exact HLS video resume requires implemented AAC audio transcode";
            return result;
        }
        result.audioAction = MediaTrackAction::Transcode;
        result.adaptationClass = MediaAdaptationClass::Transcode;
    }

    result.reason =
        "exact HLS video resume decodes selected A/V tracks for synchronized start";
    return result;
}
