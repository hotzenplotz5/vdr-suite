#include "DeviceMediaCapabilityProfile.h"
#include <cassert>

namespace
{
MediaSourceDescriptor source(MediaCodec video, MediaCodec audio)
{
    MediaSourceDescriptor s;
    s.container = MediaContainer::MpegTs;
    s.seekable = true;
    s.videoStreams.push_back({video, 1920, 1080, 25.0, false, false});
    s.audioStreams.push_back({audio, 2, "deu"});
    return s;
}

ClientMediaCapabilities advertised()
{
    ClientMediaCapabilities c;
    c.protocols = {MediaDeliveryProtocol::Hls};
    c.containers = {MediaContainer::Fmp4};
    c.videoCodecs = {MediaCodec::H264, MediaCodec::H265};
    c.audioCodecs = {MediaCodec::Aac, MediaCodec::Ac3};
    c.maxVideoWidth = 1920;
    c.maxVideoHeight = 1080;
    c.maxAudioChannels = 2;
    return c;
}

MediaPlaybackPathEvidence path(MediaCodec v, MediaCodec a)
{
    MediaPlaybackPathEvidence e;
    e.engine = MediaPlaybackEngine::MediaSourceExtensions;
    e.grade = MediaEvidenceGrade::VerifiedPlayback;
    e.protocol = MediaDeliveryProtocol::Hls;
    e.container = MediaContainer::Fmp4;
    e.videoCodec = v;
    e.audioCodec = a;
    e.pictureObserved = true;
    e.soundObserved = true;
    e.verifiedVideoWidth = 1920;
    e.verifiedVideoHeight = 1080;
    e.verifiedAudioChannels = 2;
    return e;
}
}

int main()
{
    DeviceMediaProfileKey key{
        "dev_123", "b20261011.24", "vidaa_9", "mse_v1",
        MediaPlaybackEngine::MediaSourceExtensions};
    DeviceMediaCapabilityProfile profile;
    profile.key = key;
    profile.verifiedPaths = {path(MediaCodec::H264, MediaCodec::Aac)};
    DeviceMediaProfileSelector selector;
    const auto h264 = selector.select(
        source(MediaCodec::H264, MediaCodec::Aac), advertised(), profile, key);
    assert(h264.available && h264.presentation.videoAction == MediaTrackAction::Copy);
    assert(h264.presentation.audioAction == MediaTrackAction::Copy);
    const auto ac3 = selector.select(
        source(MediaCodec::H264, MediaCodec::Ac3), advertised(), profile, key);
    assert(ac3.available && ac3.presentation.videoAction == MediaTrackAction::Copy);
    assert(ac3.presentation.audioAction == MediaTrackAction::Transcode);
    const auto hevc = selector.select(
        source(MediaCodec::H265, MediaCodec::Aac), advertised(), profile, key);
    assert(hevc.available && hevc.presentation.videoAction == MediaTrackAction::Transcode);
    assert(hevc.presentation.targetVideoCodec == MediaCodec::H264);

    auto wrong = key;
    wrong.deviceId = "dev_other";
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, wrong).available);
    wrong = key;
    wrong.firmwareVersion = "vidaa_10";
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, wrong).available);
    wrong = key;
    wrong.appVersion = "b20261011.25";
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, wrong).available);
    wrong = key;
    wrong.engine = MediaPlaybackEngine::Native;
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, wrong).available);

    profile.verifiedPaths[0].grade = MediaEvidenceGrade::RuntimeReported;
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, key).available);
    profile.verifiedPaths[0].grade = MediaEvidenceGrade::VerifiedPlayback;
    profile.verifiedPaths[0].soundObserved = false;
    assert(!selector.select(source(MediaCodec::H264, MediaCodec::Aac),
        advertised(), profile, key).available);
    profile.verifiedPaths[0].soundObserved = true;

    // Native HEVC copy demands exact profile and bit depth *in both*
    // source metadata and observed playback evidence. Codec name alone fails.
    profile.verifiedPaths = {path(MediaCodec::H265, MediaCodec::Aac)};
    auto hevcSource = source(MediaCodec::H265, MediaCodec::Aac);
    assert(!selector.select(hevcSource, advertised(), profile, key).available);
    hevcSource.videoStreams[0].codecProfile = "Main 10";
    hevcSource.videoStreams[0].bitDepth = 10;
    profile.verifiedPaths[0].videoProfile = "Main";
    profile.verifiedPaths[0].verifiedVideoBitDepth = 10;
    assert(!selector.select(hevcSource, advertised(), profile, key).available);
    profile.verifiedPaths[0].videoProfile = "Main 10";
    assert(selector.select(hevcSource, advertised(), profile, key).available);
    hevcSource.videoStreams[0].hdr = true;
    hevcSource.videoStreams[0].colorTransfer = "smpte2084";
    assert(!selector.select(hevcSource, advertised(), profile, key).available);
    profile.verifiedPaths[0].hdrOutputObserved = true;
    profile.verifiedPaths[0].hdrTransfer = "smpte2084";
    assert(selector.select(hevcSource, advertised(), profile, key).available);
    profile.verifiedPaths[0].hdrTransfer = "arib-std-b67";
    assert(!selector.select(hevcSource, advertised(), profile, key).available);

    // Two authenticated devices negotiate the same Recording independently.
    // Kodi's external engine with verified byte ranges can direct-play TS
    // while the VIDAA MSE engine requires HLS-fMP4 and audio-only transcode.
    DeviceMediaProfileKey kodiKey{
        "dev_kodi", "kodi22", "android14", "videoplayer_v1",
        MediaPlaybackEngine::External};
    DeviceMediaCapabilityProfile kodiProfile;
    kodiProfile.key = kodiKey;
    auto kodiPath = path(MediaCodec::H264, MediaCodec::Ac3);
    kodiPath.engine = MediaPlaybackEngine::External;
    kodiPath.protocol = MediaDeliveryProtocol::Progressive;
    kodiPath.container = MediaContainer::MpegTs;
    kodiPath.byteRangeHttpObserved = true;
    kodiProfile.verifiedPaths = {kodiPath};
    ClientMediaCapabilities kodiCaps;
    kodiCaps.protocols = {MediaDeliveryProtocol::Progressive};
    kodiCaps.containers = {MediaContainer::MpegTs};
    kodiCaps.videoCodecs = {MediaCodec::H264};
    kodiCaps.audioCodecs = {MediaCodec::Ac3};
    kodiCaps.supportsByteRanges = true;
    kodiCaps.maxVideoWidth = 3840;
    kodiCaps.maxVideoHeight = 2160;
    kodiCaps.maxAudioChannels = 6;
    const auto shared = source(MediaCodec::H264, MediaCodec::Ac3);
    const auto kodiSelected = selector.select(shared, kodiCaps, kodiProfile, kodiKey);
    assert(kodiSelected.available);
    assert(kodiSelected.presentation.profileId == "progressive-direct");
    assert(kodiSelected.presentation.adaptationClass == MediaAdaptationClass::PassThrough);
    assert(kodiSelected.presentation.videoAction == MediaTrackAction::Copy);
    assert(kodiSelected.presentation.audioAction == MediaTrackAction::Copy);
    profile.verifiedPaths = {path(MediaCodec::H264, MediaCodec::Aac)};
    const auto vidaaSelected = selector.select(shared, advertised(), profile, key);
    assert(vidaaSelected.available);
    assert(vidaaSelected.presentation.profileId == "hls-fmp4");
    assert(vidaaSelected.presentation.videoAction == MediaTrackAction::Copy);
    assert(vidaaSelected.presentation.audioAction == MediaTrackAction::Transcode);
    assert(!selector.select(shared, kodiCaps, profile, kodiKey).available);

    profile.version = 2;
    assert(!selector.select(hevcSource, advertised(), profile, key).available);
    profile.version = 1;
    profile.key.deviceId = "../untrusted";
    assert(!selector.select(hevcSource, advertised(), profile, profile.key).available);
}
