#include "MediaCapabilityEvidence.h"

#include <cassert>

namespace
{
ClientMediaCapabilities advertised()
{
    ClientMediaCapabilities caps;
    caps.protocols = {MediaDeliveryProtocol::Hls, MediaDeliveryProtocol::Progressive};
    caps.containers = {MediaContainer::Fmp4, MediaContainer::MpegTs};
    caps.videoCodecs = {MediaCodec::H264, MediaCodec::H265};
    caps.audioCodecs = {MediaCodec::Aac, MediaCodec::Ac3};
    caps.supportsByteRanges = true;
    caps.maxVideoWidth = 3840;
    caps.maxVideoHeight = 2160;
    caps.maxAudioChannels = 6;
    return caps;
}

MediaPlaybackPathEvidence hisenseVerifiedRecording()
{
    MediaPlaybackPathEvidence evidence;
    evidence.engine = MediaPlaybackEngine::MediaSourceExtensions;
    evidence.grade = MediaEvidenceGrade::VerifiedPlayback;
    evidence.protocol = MediaDeliveryProtocol::Hls;
    evidence.container = MediaContainer::Fmp4;
    evidence.videoCodec = MediaCodec::H264;
    evidence.audioCodec = MediaCodec::Aac;
    evidence.pictureObserved = true;
    evidence.soundObserved = true;
    evidence.verifiedVideoWidth = 1920;
    evidence.verifiedVideoHeight = 1080;
    evidence.verifiedAudioChannels = 2;
    return evidence;
}

MediaVerifiedPath narrow(
    const ClientMediaCapabilities& caps, const MediaPlaybackPathEvidence& evidence,
    MediaPlaybackEngine engine = MediaPlaybackEngine::MediaSourceExtensions,
    MediaDeliveryProtocol protocol = MediaDeliveryProtocol::Hls,
    MediaContainer container = MediaContainer::Fmp4,
    MediaCodec video = MediaCodec::H264, MediaCodec audio = MediaCodec::Aac,
    bool requiresHdr = false)
{
    return narrowToVerifiedPath(
        caps, evidence, engine, protocol, container, video, audio, requiresHdr);
}
} // namespace

int main()
{
    const ClientMediaCapabilities caps = advertised();
    const MediaPlaybackPathEvidence verified = hisenseVerifiedRecording();
    {
        const auto result = narrow(caps, verified);
        assert(result.available);
        assert(result.capabilities.protocols.size() == 1);
        assert(result.capabilities.protocols.front() == MediaDeliveryProtocol::Hls);
        assert(result.capabilities.containers.size() == 1);
        assert(result.capabilities.containers.front() == MediaContainer::Fmp4);
        assert(result.capabilities.videoCodecs.size() == 1);
        assert(result.capabilities.videoCodecs.front() == MediaCodec::H264);
        assert(result.capabilities.audioCodecs.size() == 1);
        assert(result.capabilities.audioCodecs.front() == MediaCodec::Aac);
        assert(result.capabilities.maxVideoWidth == 1920);
        assert(result.capabilities.maxVideoHeight == 1080);
        assert(result.capabilities.maxAudioChannels == 2);
        // MSE seek success does not prove HTTP byte-range support.
        assert(!result.capabilities.supportsByteRanges);
    }

    // Runtime APIs and untrusted client profile hints cannot prove decoding.
    for (auto grade : {MediaEvidenceGrade::Unknown, MediaEvidenceGrade::ProfileHint,
             MediaEvidenceGrade::RuntimeReported, MediaEvidenceGrade::ObservedFailure}) {
        auto report = verified;
        report.grade = grade;
        assert(!narrow(caps, report).available);
    }

    {
        auto report = verified;
        report.pictureObserved = false;
        assert(!narrow(caps, report).available);
    }
    {
        auto report = verified;
        report.soundObserved = false;
        assert(!narrow(caps, report).available);
    }

    // Never manufacture HEVC, native TS, AC-3 or Kodi/external capability from
    // success of a single H264/AAC MSE/fMP4 recording on the Hisense.
    assert(!narrow(caps, verified, MediaPlaybackEngine::Native).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::External).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::MediaSourceExtensions,
        MediaDeliveryProtocol::Progressive).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::MediaSourceExtensions,
        MediaDeliveryProtocol::Hls, MediaContainer::MpegTs).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::MediaSourceExtensions,
        MediaDeliveryProtocol::Hls, MediaContainer::Fmp4, MediaCodec::H265).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::MediaSourceExtensions,
        MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
        MediaCodec::H264, MediaCodec::Ac3).available);
    assert(!narrow(caps, verified, MediaPlaybackEngine::MediaSourceExtensions,
        MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
        MediaCodec::H264, MediaCodec::Aac, true).available);

    {
        auto report = verified;
        report.hdrOutputObserved = true;
        assert(narrow(caps, report, MediaPlaybackEngine::MediaSourceExtensions,
            MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
            MediaCodec::H264, MediaCodec::Aac, true).available);
    }
    {
        auto report = verified;
        report.verifiedAudioChannels = 0;
        assert(!narrow(caps, report).available);
        report.verifiedAudioChannels = 20;
        assert(narrow(caps, report).capabilities.maxAudioChannels == 6);
    }
    {
        auto report = verified;
        report.verifiedVideoWidth = 0;
        assert(!narrow(caps, report).available);
        report.verifiedVideoWidth = 8192;
        assert(narrow(caps, report).capabilities.maxVideoWidth == 3840);
    }
    {
        auto report = verified;
        report.byteRangeHttpObserved = true;
        assert(narrow(caps, report).capabilities.supportsByteRanges);
    }

    // Unknown and non-media tokens cannot be promoted even by a mislabeled
    // success report. Other codec combinations are independent rows.
    {
        auto report = verified;
        report.videoCodec = MediaCodec::Unknown;
        assert(!narrow(caps, report, MediaPlaybackEngine::MediaSourceExtensions,
            MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
            MediaCodec::Unknown, MediaCodec::Aac).available);
        report = verified;
        report.audioCodec = MediaCodec::None;
        assert(!narrow(caps, report, MediaPlaybackEngine::MediaSourceExtensions,
            MediaDeliveryProtocol::Hls, MediaContainer::Fmp4,
            MediaCodec::H264, MediaCodec::None).available);
    }

    // Never permit an outcome to add any capability absent from the declared
    // negotiated client baseline.
    {
        auto c = caps;
        c.containers = {MediaContainer::MpegTs};
        assert(!narrow(c, verified).available);
        c = caps;
        c.videoCodecs = {MediaCodec::H265};
        assert(!narrow(c, verified).available);
        c = caps;
        c.audioCodecs = {MediaCodec::Ac3};
        assert(!narrow(c, verified).available);
        c = caps;
        c.protocols = {MediaDeliveryProtocol::Progressive};
        assert(!narrow(c, verified).available);
    }
}
