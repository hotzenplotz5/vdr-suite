#include "MediaPresentationSelector.h"

#include <algorithm>
#include <cstddef>

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

int firstDirectVideoIndex(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client)
{
    for (std::size_t index = 0; index < source.videoStreams.size(); ++index) {
        const auto& video = source.videoStreams[index];
        if (validSourceCodec(video.codec) &&
            contains(client.videoCodecs, video.codec) &&
            fitsVideoLimits(video, client)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int firstDirectAudioIndex(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client)
{
    for (std::size_t index = 0; index < source.audioStreams.size(); ++index) {
        const auto& audio = source.audioStreams[index];
        if (validSourceCodec(audio.codec) &&
            contains(client.audioCodecs, audio.codec)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int firstValidVideoIndex(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& client)
{
    for (std::size_t index = 0; index < source.videoStreams.size(); ++index) {
        const auto& video = source.videoStreams[index];
        if (validSourceCodec(video.codec) && fitsVideoLimits(video, client)) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

int firstValidAudioIndex(const MediaSourceDescriptor& source)
{
    for (std::size_t index = 0; index < source.audioStreams.size(); ++index) {
        if (validSourceCodec(source.audioStreams[index].codec)) {
            return static_cast<int>(index);
        }
    }
    return -1;
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

    const int videoIndex = firstDirectVideoIndex(source, client);
    if (!source.videoStreams.empty() && videoIndex < 0) {
        return unavailable("no directly playable source video track is available");
    }

    const int audioIndex = firstDirectAudioIndex(source, client);
    if (!source.audioStreams.empty() && audioIndex < 0) {
        return unavailable("no directly playable source audio track is available");
    }

    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "progressive-direct";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = source.container;
    profile.adaptationClass = MediaAdaptationClass::PassThrough;
    profile.sourceVideoStreamIndex = videoIndex;
    profile.sourceAudioStreamIndex = audioIndex;
    profile.videoAction = videoIndex < 0
        ? MediaTrackAction::Omit
        : MediaTrackAction::Copy;
    profile.audioAction = audioIndex < 0
        ? MediaTrackAction::Omit
        : MediaTrackAction::Copy;
    profile.targetVideoCodec = videoIndex < 0
        ? MediaCodec::None
        : source.videoStreams[static_cast<std::size_t>(videoIndex)].codec;
    profile.targetAudioCodec = audioIndex < 0
        ? MediaCodec::None
        : source.audioStreams[static_cast<std::size_t>(audioIndex)].codec;
    profile.reason = "client can consume one selected source video/audio track without transformation";
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
        int videoIndex = firstDirectVideoIndex(source, client);
        if (videoIndex >= 0) {
            profile.sourceVideoStreamIndex = videoIndex;
            profile.videoAction = MediaTrackAction::Copy;
            profile.targetVideoCodec =
                source.videoStreams[static_cast<std::size_t>(videoIndex)].codec;
        }
        else {
            videoIndex = firstValidVideoIndex(source, client);
            if (videoIndex < 0) {
                return unavailable("no valid source video track fits the requested client limits");
            }
            if (!contains(client.videoCodecs, MediaCodec::H264)) {
                return unavailable("no allowed video codec path is available");
            }
            profile.sourceVideoStreamIndex = videoIndex;
            profile.videoAction = MediaTrackAction::Transcode;
            profile.targetVideoCodec = MediaCodec::H264;
            profile.adaptationClass = MediaAdaptationClass::Transcode;
        }
    }

    if (source.audioStreams.empty()) {
        profile.audioAction = MediaTrackAction::Omit;
        profile.targetAudioCodec = MediaCodec::None;
    }
    else {
        int audioIndex = firstDirectAudioIndex(source, client);
        if (audioIndex >= 0) {
            profile.sourceAudioStreamIndex = audioIndex;
            profile.audioAction = MediaTrackAction::Copy;
            profile.targetAudioCodec =
                source.audioStreams[static_cast<std::size_t>(audioIndex)].codec;
        }
        else {
            audioIndex = firstValidAudioIndex(source);
            if (audioIndex < 0) {
                return unavailable("source audio codec is unknown");
            }
            if (!contains(client.audioCodecs, MediaCodec::Aac)) {
                return unavailable("no allowed audio codec path is available");
            }
            profile.sourceAudioStreamIndex = audioIndex;
            profile.audioAction = MediaTrackAction::Transcode;
            profile.targetAudioCodec = MediaCodec::Aac;
            profile.adaptationClass = MediaAdaptationClass::Transcode;
        }
    }

    profile.reason = profile.adaptationClass == MediaAdaptationClass::Transcode
        ? "HLS packaging selected with only the selected incompatible tracks transcoded"
        : "HLS packaging selected with compatible selected tracks copied";
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