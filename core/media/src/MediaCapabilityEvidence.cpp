#include "MediaCapabilityEvidence.h"

#include <algorithm>

namespace
{

template <typename T>
bool contains(const std::vector<T>& values, T value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool isVideo(MediaCodec codec)
{
    return codec == MediaCodec::H264 || codec == MediaCodec::H265 ||
        codec == MediaCodec::Mpeg2Video;
}

bool isAudio(MediaCodec codec)
{
    return codec == MediaCodec::Aac || codec == MediaCodec::Ac3 ||
        codec == MediaCodec::Eac3 || codec == MediaCodec::MpegAudio ||
        codec == MediaCodec::Dts;
}

int limitToEvidence(int advertised, int observed)
{
    return advertised > 0 ? std::min(advertised, observed) : observed;
}

} // namespace

MediaVerifiedPath narrowToVerifiedPath(
    const ClientMediaCapabilities& advertised,
    const MediaPlaybackPathEvidence& observed,
    MediaPlaybackEngine selectedEngine,
    MediaDeliveryProtocol requestedProtocol,
    MediaContainer requestedContainer,
    MediaCodec requestedVideoCodec,
    MediaCodec requestedAudioCodec,
    bool requiresHdrOutput)
{
    MediaVerifiedPath result;
    if (observed.grade != MediaEvidenceGrade::VerifiedPlayback ||
        observed.engine != selectedEngine ||
        observed.protocol != requestedProtocol ||
        observed.container != requestedContainer ||
        observed.videoCodec != requestedVideoCodec ||
        observed.audioCodec != requestedAudioCodec ||
        !isVideo(requestedVideoCodec) || !isAudio(requestedAudioCodec) ||
        !observed.pictureObserved || !observed.soundObserved ||
        (requiresHdrOutput && !observed.hdrOutputObserved) ||
        observed.verifiedVideoWidth <= 0 ||
        observed.verifiedVideoHeight <= 0 ||
        observed.verifiedAudioChannels <= 0 ||
        !contains(advertised.protocols, requestedProtocol) ||
        !contains(advertised.containers, requestedContainer) ||
        !contains(advertised.videoCodecs, requestedVideoCodec) ||
        !contains(advertised.audioCodecs, requestedAudioCodec)) {
        return result;
    }

    result.capabilities.protocols = {requestedProtocol};
    result.capabilities.containers = {requestedContainer};
    result.capabilities.videoCodecs = {requestedVideoCodec};
    result.capabilities.audioCodecs = {requestedAudioCodec};
    result.capabilities.maxVideoWidth = limitToEvidence(
        advertised.maxVideoWidth, observed.verifiedVideoWidth);
    result.capabilities.maxVideoHeight = limitToEvidence(
        advertised.maxVideoHeight, observed.verifiedVideoHeight);
    result.capabilities.maxAudioChannels = limitToEvidence(
        advertised.maxAudioChannels, observed.verifiedAudioChannels);
    // A successful buffered seek is not an HTTP Range capability.
    result.capabilities.supportsByteRanges =
        advertised.supportsByteRanges && observed.byteRangeHttpObserved;
    result.available = true;
    return result;
}
