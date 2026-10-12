#include "MediaPresentationSelector.h"
#include "MediaCapabilityEvidence.h"

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

ClientMediaCapabilities browserProgressiveCapabilities()
{
    ClientMediaCapabilities client = browserHlsCapabilities();
    client.protocols = {MediaDeliveryProtocol::Progressive};
    client.supportsByteRanges = false;
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
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.sourceAudioStreamIndex == 0);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 1080);
        assert(!profile.deinterlaceVideo);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::None);
        assert(profile.targetAudioChannels == 6);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.push_back({MediaCodec::Aac, 2, "eng"});

        ClientMediaCapabilities client;
        client.protocols = {MediaDeliveryProtocol::Progressive};
        client.containers = {MediaContainer::MpegTs};
        client.videoCodecs = {MediaCodec::H264};
        client.audioCodecs = {MediaCodec::Aac};
        client.supportsByteRanges = true;
        client.maxAudioChannels = 2;

        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::PassThrough);
        assert(profile.sourceAudioStreamIndex == 1);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        const MediaSourceDescriptor source = h264Ac3Recording();
        const auto profile = selector.select(source, browserProgressiveCapabilities());
        assert(profile.available);
        assert(profile.profileId == "progressive-fmp4");
        assert(profile.protocol == MediaDeliveryProtocol::Progressive);
        assert(profile.container == MediaContainer::Fmp4);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.front().codec = MediaCodec::Aac;
        source.audioStreams.front().channels = 2;
        const auto profile = selector.select(source, browserProgressiveCapabilities());
        assert(profile.available);
        assert(profile.profileId == "progressive-fmp4");
        assert(profile.adaptationClass == MediaAdaptationClass::Remux);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.audioAction == MediaTrackAction::Copy);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.growing = true;
        source.resourceKind = MediaResourceKind::GrowingRecording;
        source.seekable = false;
        ClientMediaCapabilities client = browserProgressiveCapabilities();
        client.protocols.push_back(MediaDeliveryProtocol::Hls);
        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.protocol == MediaDeliveryProtocol::Hls);
    }

    {
        const MediaSourceDescriptor source = h264Ac3Recording();
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.container == MediaContainer::Fmp4);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 1080);
        assert(!profile.deinterlaceVideo);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::None);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.sourceAudioStreamIndex == 0);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front().interlaced = true;

        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 1080);
        assert(profile.deinterlaceVideo);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::Deinterlace);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.front().channels = 2;
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.front().codec = MediaCodec::Aac;
        source.audioStreams.front().channels = 2;
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::Remux);
        assert(profile.audioAction == MediaTrackAction::Copy);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.front().codec = MediaCodec::Aac;
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.front().codec = MediaCodec::Aac;
        source.audioStreams.front().channels = 0;
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.sourceAudioStreamIndex == 0);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.push_back({MediaCodec::Aac, 2, "eng"});
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.adaptationClass == MediaAdaptationClass::Remux);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.audioAction == MediaTrackAction::Copy);
        assert(profile.sourceAudioStreamIndex == 1);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.push_back({MediaCodec::Aac, 2, "eng"});
        source.audioStreams[1].defaultTrack = true;
        const auto profile = selector.select(source, browserProgressiveCapabilities());
        assert(profile.available);
        assert(profile.profileId == "progressive-fmp4");
        assert(profile.sourceAudioStreamIndex == 1);
        assert(profile.audioAction == MediaTrackAction::Copy);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.push_back({MediaCodec::Aac, 2, "eng"});
        source.audioStreams[1].defaultTrack = true;
        const auto profile = selector.select(source, browserProgressiveCapabilities(), 0);
        assert(profile.available);
        assert(profile.profileId == "progressive-fmp4");
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.sourceAudioStreamIndex == 0);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.audioStreams.push_back({MediaCodec::Aac, 2, "eng"});
        const auto profile = selector.select(source, browserProgressiveCapabilities(), 1);
        assert(profile.available);
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.sourceAudioStreamIndex == 1);
        assert(profile.audioAction == MediaTrackAction::Copy);
    }

    {
        const auto profile = selector.select(
            h264Ac3Recording(), browserProgressiveCapabilities(), 4);
        assert(!profile.available);
        assert(profile.reason == "requested audio track is unavailable");
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        ClientMediaCapabilities client = browserHlsCapabilities();
        client.maxAudioChannels = 6;

        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 6);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front().codec = MediaCodec::Mpeg2Video;

        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 1080);
        assert(!profile.deinterlaceVideo);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::Standard);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front() = {MediaCodec::H265, 3840, 2160, 23.976, false};
        source.audioStreams.clear();
        source.audioStreams.push_back({MediaCodec::Aac, 6, "ger"});
        source.audioStreams.push_back({MediaCodec::Aac, 8, "eng"});

        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.profileId == "hls-fmp4");
        assert(profile.adaptationClass == MediaAdaptationClass::Transcode);
        assert(profile.sourceVideoStreamIndex == 0);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.targetVideoCodec == MediaCodec::H264);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 1080);
        assert(!profile.deinterlaceVideo);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::UhdSource);
        assert(profile.sourceAudioStreamIndex == 0);
        assert(profile.audioAction == MediaTrackAction::Transcode);
        assert(profile.targetAudioCodec == MediaCodec::Aac);
        assert(profile.targetAudioChannels == 2);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front() = {MediaCodec::H265, 3840, 1600, 24.0, false};
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::UhdSource);
        assert(profile.targetVideoWidth == 1920);
        assert(profile.targetVideoHeight == 800);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front() = {MediaCodec::H265, 1280, 720, 24.0, false};
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(profile.available);
        assert(profile.videoAction == MediaTrackAction::Transcode);
        assert(profile.videoTranscodeWorkload == MediaTranscodeWorkload::Standard);
        assert(profile.targetVideoWidth == 1280);
        assert(profile.targetVideoHeight == 720);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        source.videoStreams.front() = {MediaCodec::H265, 0, 0, 24.0, false};
        const auto profile = selector.select(source, browserHlsCapabilities());
        assert(!profile.available);
    }

    {
        MediaSourceDescriptor source = h264Ac3Recording();
        ClientMediaCapabilities client = browserHlsCapabilities();
        client.containers = {MediaContainer::MpegTs};
        client.audioCodecs = {MediaCodec::Ac3};
        client.maxAudioChannels = 6;

        const auto profile = selector.select(source, client);
        assert(profile.available);
        assert(profile.profileId == "hls-ts");
        assert(profile.adaptationClass == MediaAdaptationClass::Remux);
        assert(profile.videoAction == MediaTrackAction::Copy);
        assert(profile.audioAction == MediaTrackAction::Copy);
        assert(profile.targetAudioChannels == 6);
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

    // Future Kodi/Android/VIDAA adapters use the same exact evidence tuple.
    // Reported MSE support is not observed decoded H.265/AC3 playback.
    {
        ClientMediaCapabilities claimed = browserHlsCapabilities();
        claimed.videoCodecs.push_back(MediaCodec::H265);
        claimed.audioCodecs.push_back(MediaCodec::Ac3);
        claimed.containers.push_back(MediaContainer::MpegTs);
        MediaPlaybackPathEvidence evidence;
        evidence.engine = MediaPlaybackEngine::MediaSourceExtensions;
        evidence.protocol = MediaDeliveryProtocol::Hls;
        evidence.container = MediaContainer::Fmp4;
        evidence.videoCodec = MediaCodec::H264;
        evidence.audioCodec = MediaCodec::Aac;
        evidence.verifiedVideoWidth = 1920;
        evidence.verifiedVideoHeight = 1080;
        evidence.verifiedAudioChannels = 2;
        evidence.pictureObserved = true;
        evidence.soundObserved = true;

        const auto evaluate = [&](MediaDeliveryProtocol protocol,
                                  MediaContainer container,
                                  MediaCodec video, MediaCodec audio) {
            return narrowToVerifiedPath(
                claimed, evidence, MediaPlaybackEngine::MediaSourceExtensions,
                protocol, container, video, audio);
        };

        evidence.grade = MediaEvidenceGrade::RuntimeReported;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
        evidence.grade = MediaEvidenceGrade::ProfileHint;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
        evidence.grade = MediaEvidenceGrade::ObservedFailure;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
        evidence.grade = MediaEvidenceGrade::VerifiedPlayback;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::MpegTs,
                         MediaCodec::H264, MediaCodec::Aac).available);
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H265, MediaCodec::Aac).available);
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Ac3).available);
        assert(!evaluate(MediaDeliveryProtocol::Progressive, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
        assert(!narrowToVerifiedPath(
            claimed, evidence, MediaPlaybackEngine::Native,
            MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
            MediaCodec::H264, MediaCodec::Aac).available);
        assert(!narrowToVerifiedPath(
            claimed, evidence, MediaPlaybackEngine::MediaSourceExtensions,
            MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
            MediaCodec::H264, MediaCodec::Aac, true).available);

        evidence.soundObserved = false;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
        evidence.soundObserved = true;
        const auto verified = evaluate(MediaDeliveryProtocol::Hls,
            MediaContainer::Fmp4, MediaCodec::H264, MediaCodec::Aac);
        assert(verified.available);
        assert(verified.capabilities.videoCodecs.size() == 1);
        assert(verified.capabilities.videoCodecs.front() == MediaCodec::H264);
        assert(verified.capabilities.audioCodecs.size() == 1);
        assert(verified.capabilities.audioCodecs.front() == MediaCodec::Aac);
        assert(verified.capabilities.containers.size() == 1);
        assert(verified.capabilities.containers.front() == MediaContainer::Fmp4);
        assert(!verified.capabilities.supportsByteRanges);

        // One verified H.264/AAC MSE path cannot silently copy an untested
        // HEVC video track or AC3 audio track.
        const auto selected = selector.select(
            h264Ac3Recording(), verified.capabilities);
        assert(selected.available);
        assert(selected.profileId == "hls-fmp4");
        assert(selected.videoAction == MediaTrackAction::Copy);
        assert(selected.targetVideoCodec == MediaCodec::H264);
        assert(selected.audioAction == MediaTrackAction::Transcode);
        assert(selected.targetAudioCodec == MediaCodec::Aac);
        MediaSourceDescriptor hevc = h264Ac3Recording();
        hevc.videoStreams.front().codec = MediaCodec::H265;
        const auto adapted = selector.select(hevc, verified.capabilities);
        assert(adapted.available);
        assert(adapted.videoAction == MediaTrackAction::Transcode);
        assert(adapted.targetVideoCodec == MediaCodec::H264);

        evidence.verifiedVideoWidth = 1280;
        evidence.verifiedVideoHeight = 720;
        evidence.verifiedAudioChannels = 1;
        const auto bounded = evaluate(MediaDeliveryProtocol::Hls,
            MediaContainer::Fmp4, MediaCodec::H264, MediaCodec::Aac);
        assert(bounded.available);
        assert(bounded.capabilities.maxVideoWidth == 1280);
        assert(bounded.capabilities.maxVideoHeight == 720);
        assert(bounded.capabilities.maxAudioChannels == 1);
        evidence.verifiedVideoWidth = 0;
        assert(!evaluate(MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
                         MediaCodec::H264, MediaCodec::Aac).available);
    }

    return 0;
}
