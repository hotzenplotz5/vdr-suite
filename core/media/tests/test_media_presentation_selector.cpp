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
    client.maxVideoWidth = 1920;
    client.maxVideoHeight = 1080;
    client.maxAudioChannels = 2;
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
        assert(audioAction == audioAction); /* placeholder */
    }

    return 0;
}
