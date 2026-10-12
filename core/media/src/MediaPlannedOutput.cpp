#include "MediaPlannedOutput.h"

#include <cstddef>

namespace
{

bool known(MediaCodec codec)
{
    return codec != MediaCodec::Unknown && codec != MediaCodec::None;
}

template <typename T>
bool indexValid(int index, const std::vector<T>& streams)
{
    return index >= 0 &&
        static_cast<std::size_t>(index) < streams.size();
}

} // namespace

MediaPlannedOutput projectPlannedOutput(
    const MediaSourceDescriptor& source,
    const MediaPresentationProfile& profile)
{
    MediaPlannedOutput invalid;
    if (!profile.available || profile.profileId.empty() ||
        profile.container == MediaContainer::Unknown)
        return invalid;

    MediaPlannedOutput output;
    output.protocol = profile.protocol;
    output.container = profile.container;
    output.adaptationClass = profile.adaptationClass;
    output.videoAction = profile.videoAction;
    output.audioAction = profile.audioAction;

    if (profile.videoAction == MediaTrackAction::Omit) {
        if (!source.videoStreams.empty() ||
            profile.sourceVideoStreamIndex != -1 ||
            profile.targetVideoCodec != MediaCodec::None)
            return invalid;
    } else {
        if (!indexValid(profile.sourceVideoStreamIndex, source.videoStreams))
            return invalid;
        const auto& video = source.videoStreams[
            static_cast<std::size_t>(profile.sourceVideoStreamIndex)];
        if (!known(video.codec))
            return invalid;
        output.sourceVideoStreamIndex = profile.sourceVideoStreamIndex;
        output.sourceVideoCodec = video.codec;
        output.sourceVideoHdr = video.hdr;
        if (profile.videoAction == MediaTrackAction::Copy) {
            if (profile.targetVideoCodec != video.codec ||
                profile.targetVideoWidth != video.width ||
                profile.targetVideoHeight != video.height ||
                video.width <= 0 || video.height <= 0)
                return invalid;
            output.targetVideoCodec = video.codec;
            output.targetVideoWidth = video.width;
            output.targetVideoHeight = video.height;
        } else if (profile.videoAction == MediaTrackAction::Transcode) {
            if (!known(profile.targetVideoCodec) ||
                profile.targetVideoWidth <= 0 ||
                profile.targetVideoHeight <= 0)
                return invalid;
            output.targetVideoCodec = profile.targetVideoCodec;
            output.targetVideoWidth = profile.targetVideoWidth;
            output.targetVideoHeight = profile.targetVideoHeight;
        } else {
            return invalid;
        }
    }

    if (profile.audioAction == MediaTrackAction::Omit) {
        if (!source.audioStreams.empty() ||
            profile.sourceAudioStreamIndex != -1 ||
            profile.targetAudioCodec != MediaCodec::None ||
            profile.targetAudioChannels != 0)
            return invalid;
    } else {
        if (!indexValid(profile.sourceAudioStreamIndex, source.audioStreams))
            return invalid;
        const auto& audio = source.audioStreams[
            static_cast<std::size_t>(profile.sourceAudioStreamIndex)];
        if (!known(audio.codec))
            return invalid;
        output.sourceAudioStreamIndex = profile.sourceAudioStreamIndex;
        output.sourceAudioCodec = audio.codec;
        if (profile.audioAction == MediaTrackAction::Copy) {
            if (profile.targetAudioCodec != audio.codec ||
                profile.targetAudioChannels != audio.channels ||
                audio.channels <= 0)
                return invalid;
            output.targetAudioCodec = audio.codec;
            output.targetAudioChannels = audio.channels;
        } else if (profile.audioAction == MediaTrackAction::Transcode) {
            if (!known(profile.targetAudioCodec) ||
                profile.targetAudioChannels < 0)
                return invalid;
            output.targetAudioCodec = profile.targetAudioCodec;
            output.targetAudioChannels = profile.targetAudioChannels;
        } else {
            return invalid;
        }
    }

    output.valid = true;
    return output;
}
