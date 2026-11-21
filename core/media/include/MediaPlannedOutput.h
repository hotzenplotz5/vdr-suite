#pragma once

#include "MediaCapabilities.h"

// Trusted internal projection of the selected output plan. This describes
// the selected profile; it is not proof of actual encoded worker output or
// proof that the device decoded the stream.
struct MediaPlannedOutput
{
    bool valid = false;
    MediaDeliveryProtocol protocol = MediaDeliveryProtocol::Progressive;
    MediaContainer container = MediaContainer::Unknown;
    MediaAdaptationClass adaptationClass = MediaAdaptationClass::PassThrough;
    MediaCodec sourceVideoCodec = MediaCodec::None;
    MediaCodec sourceAudioCodec = MediaCodec::None;
    MediaCodec targetVideoCodec = MediaCodec::None;
    MediaCodec targetAudioCodec = MediaCodec::None;
    MediaTrackAction videoAction = MediaTrackAction::Omit;
    MediaTrackAction audioAction = MediaTrackAction::Omit;
    int sourceVideoStreamIndex = -1;
    int sourceAudioStreamIndex = -1;
    int targetVideoWidth = 0;
    int targetVideoHeight = 0;
    int targetAudioChannels = 0;
    bool sourceVideoHdr = false; // source fact only; HDR *output* is not verified.
};

// Never call this on a raw client report: source/selection must come from the
// trusted source resolver and MediaPresentationSelector after policy validation.
MediaPlannedOutput projectPlannedOutput(
    const MediaSourceDescriptor& trustedSource,
    const MediaPresentationProfile& selectedProfile);
