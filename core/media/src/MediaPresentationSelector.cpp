#include "MediaPresentationSelector.h"

#include <algorithm>

namespace
{

template <typename T>
bool contains(const std::vector<T>& values, T wanted)
{
    return std::find(values.begin(), values.end(), wanted) != values.end();
}

bool validSourceCodec(MediaCodec codec)
{
    return codec != MediaCodec::Unknown && codec != MediaCodec::None;
}

bool fitsVideoLimits(
    const MediaVideoStreamDescriptor& video,
    const ClientMediaCapabilities& client)
{
    if (client.maxVideoWidth > 0 && video.width > client.maxVideoWidth) {
        return false;
    }
    if (client.maxVideoHeight > 0 && video.height > client.maxVideoHeight) {
        return false;
    }
    return true;
}

MediaPresentationProfile unavailable(const std::string& reason)
{
    MediaPresentationProfile profile;
    profile.available = false;
    profile.reason = reason;
    return profile;
}

MediaPresentationProfile directProfile(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client)
{
    if (!contains(client.protocols, MediaDeliveryProtocol::Progressive) ||
        !contains(client.containers, source.container)) {
        return unavailable("progressive transport or source container unsupported");
    }

    if (source.seekable && !client.supportsByteRanges) {
        return unavailable("seekable recording requires byte-range support");
    }

    for (const auto& video : source.videoStreams) {
        if (!validSourceCodec(video.codec) ||
            !contains(client.videoCodecs, video.codec) ||
            !fitsVideoLimits(video, client)) {
            return unavailable("source video stream is not directly playable");
        }
    }

    for (const auto& audio : source.audioStreams) {
        if (!validSourceCodec(audio.codec) ||
            !contains(client.audioCodecs, audio.codec)) {
            return unavailable("source audio stream is not directly playable");
        }
    }

    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "progressive-direct";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = source.container;
    profile.adaptationClass = MediaAdaptationClass::PassThrough;
    profile.videoAction = source.videoStreams.empty()
        ? MediaTrackAction::Omit
        : MediaTrackAction::Copy;
    profile.audioAction = source.audioStreams.empty()
        ? MediaTrackAction::Omit
        : MediaTrackAction::Copy;
    profile.targetVideoCodec = source.videoStreams.empty()
        ? MediaCodec::None
        : source.videoStreams.front().codec;
    profile.targetAudioCodec = source.audioStreams.empty()
        ? MediaCodec::None
        : source.audioStreams.front().codec;
    profile.reason = "client can consume the source transport, container and codecs directly";
    return profile;
}

MediaPresentationProfile hlsProfile(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client)
{
    if (!contains(client.protocols, MediaDeliveryProtocol::Hls)) {
        return unavailable("HLS is unsupported by the client");
    }

    MediaContainer outputContainer = MediaContainer::Unknown;
    std::string profileId;

    if (contains(client.containers, MediaContainer::Fmp4)) {
        outputContainer = MediaContainer::Fmp4;
        profileId = "hls-fmp4";
    }
    else if (contains(client.containers, MediaContainer::MpegTs)) {
        outputContainer = MediaContainer::MpegTs;
        profileId = "hls-ts";
    }
    else {
        return unavailable("client exposes no supported HLS segment container");
    }

    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = profileId;
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = outputContainer;
    profile.adaptationClass = MediaAdaptationClass::Remux;

    if (source.videoStreams.empty()) {
        profile.videoAction = MediaTrackAction::Omit;
        profile.targetVideoCodec = MediaCodec::None;
    }
    else {
        const auto& video = source.videoStreams.front();
        if (!validSourceCodec(video.codec)) {
            return unavailable("source video codec is unknown");
        }

        const bool directCodec =
            contains(client.videoCodecs, video.codec) &&
            fitsVideoLimits(video, client);

        if (directCodec) {
            profile.videoAction = MediaTrackAction::Copy;
            profile.targetVideoCodec = video.codec;
        }
        else if (contains(client.videoCodecs, MediaCodec::H264)) {
            profile.videoAction = MediaTrackAction::Transcode;
            profile.targetVideoCodec = MediaCodec::H264;
            profile.adaptationClass = MediaAdaptationClass::Transcode;
        }
        else {
            return unavailable("no allowed video codec path is available");
        }
    }

    if (source.audioStreams.empty()) {
        profile.audioAction = MediaTrackAction::Omit;
        profile.targetAudioCodec = MediaCodec::None;
    }
    else {
        const auto& audio = source.audioStreams.front();
        if (!validSourceCodec(audio.codec)) {
            return unavailable("source audio codec is unknown");
        }

        if (contains(client.audioCodecs, audio.codec)) {
            profile.audioAction = MediaTrackAction::Copy;
            profile.targetAudioCodec = audio.codec;
        }
        else if (contains(client.audioCodecs, MediaCodec::Aac)) {
            profile.audioAction = MediaTrackAction::Transcode;
            profile.targetAudioCodec = MediaCodec::Aac;
            profile.adaptationClass = MediaAdaptationClass::Transcode;
        }
        else {
            return unavailable("no allowed audio codec path is available");
        }
    }

    profile.reason = profile.adaptationClass == MediaAdaptationClass::Transcode
        ? "HLS packaging selected with only incompatible tracks transcoded"
        : "HLS packaging selected without codec re-encoding";
    return profile;
}

} // namespace

MediaPresentationProfile MediaPresentationSelector::select(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client) const
{
    if (source.container == MediaContainer::Unknown) {
        return unavailable("source container is unknown");
    }

    const MediaPresentationProfile direct = directProfile(source, client);
    if (direct.available) {
        return direct;
    }

    return hlsProfile(source, client);
}
