#pragma once

#include "MediaCapabilities.h"

// Device-independent, per playback-engine evidence. Not a Device grant,
// not a trusted assertion of FFmpeg parameters, and not a transport fallback.
enum class MediaPlaybackEngine
{
    Native,
    MediaSourceExtensions,
    External
};

enum class MediaEvidenceGrade
{
    Unknown,
    ProfileHint,
    RuntimeReported,
    VerifiedPlayback,
    ObservedFailure
};

struct MediaPlaybackPathEvidence
{
    MediaPlaybackEngine engine = MediaPlaybackEngine::Native;
    MediaEvidenceGrade grade = MediaEvidenceGrade::Unknown;
    MediaDeliveryProtocol protocol = MediaDeliveryProtocol::Hls;
    MediaContainer container = MediaContainer::Unknown;
    MediaCodec videoCodec = MediaCodec::Unknown;
    MediaCodec audioCodec = MediaCodec::Unknown;

    // A playback success requires both independently observed A/V outcomes.
    bool pictureObserved = false;
    bool soundObserved = false;
    bool hdrOutputObserved = false;
    bool byteRangeHttpObserved = false;
    int verifiedVideoWidth = 0;
    int verifiedVideoHeight = 0;
    int verifiedAudioChannels = 0;
};

struct MediaVerifiedPath
{
    bool available = false;
    ClientMediaCapabilities capabilities;
};

// Build exactly one already-observed playback combination. This does NOT
// merge independent evidence rows (which would invent unsupported codec/
// container cross-products), and does not update active session negotiation.
MediaVerifiedPath narrowToVerifiedPath(
    const ClientMediaCapabilities& advertised,
    const MediaPlaybackPathEvidence& observed,
    MediaPlaybackEngine selectedEngine,
    MediaDeliveryProtocol requestedProtocol,
    MediaContainer requestedContainer,
    MediaCodec requestedVideoCodec,
    MediaCodec requestedAudioCodec,
    bool requiresHdrOutput = false);
