#include "MediaPresentationSelector.h"

#include <cassert>

namespace
{

MediaSourceDescriptor h264Ac3Recording()
{
    MediaSourceDescriptor source;
    source.resourceKind = MediaResourceKind::Recording;
    source.container = MediaContainer::MpegTs;
    source.seekable = true;
    source.videoStreams.push_back({MediaCodec::H264, 1920, 1080, 50.0, false});
    source.audioStreams.push_back({MediaCodec::Ac3, 6, "deu"});
    return source;
}

ClientMediaCapabilities browserHlsCapabilities()
{
    ClientMediaCapabilities client;
    client.protocols = {MediaDeliveryProtocol::Hls};
    client.containers = {MediaContainer::Fmp4};
    client.videoCodecs = {MediaCodec::H264};
    client.audioCodecs = {MediaCodec::Aac};
    client.supportsByteRanges = true;
    client.maxVideoWidth = 3840;
    client.maxVideoHeight = 2160;
    return client;
}

} // namespace

int main()
{
    MediaPresentationSelector selector;

    {
        const MediaSourceDescriptor source = h264Ac3Recording();
        ClientMediaCapabilities client;
        client.protocols = {MediaDeliveryProtocol::Progressive};
        client.containers = {MediaContainer::MpegTs};
        client.videoCodecs = {MediaCodec::H264};
        client.audioCodecs = {MediaCodec::Ac3};
        client.supportsByteRanges = true;

        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.profileId == "progressive-direct");
        assert(profile.adaptationClass == MediaAdaptationClass::PassThrough);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.audioAction == MediaTrackAction::Copy);
    }

    {
        const MediaSourceDescriptor source = h264Ac3Recording();
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.container == MediaContainer::Fmp4);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front().codec = MediaCodec::Mpeg2Video;

        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        ClientMediaCapabilities client = browserHlsCapabilities();
        client.containers = {MediaContainer::MpegTs};
        client.audioCodecs = {MediaCodec::Ac3};

        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.profileId == "hls-ts");
        assert(profile.adaptationClass == MediaAdaptationClass::Remux);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.audioAction == MediaTrackAction::Copy);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front().codec = MediaCodec::Unknown;
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(!profile.available);
    }

    {
        const MediaSourceDescriptor source = h264Ac3Recording();
        ClientMediaCapabilities client;
        client.protocols = {MediaDeliveryProtocol::Progressive};
        client.containers = {MediaContainer::MpegTs};
        client.videoCodecs = {MediaCodec::H264};
        client.audioCodecs = {MediaCodec::Ac3};
        client.supportsByteRanges = false;

        const auto profile = selector.select(source, client);
        assert(!profile.available);
    }

    return 0;
}
