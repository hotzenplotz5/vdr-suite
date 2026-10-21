#include "DeviceMediaCapabilityProfile.h"

#include <algorithm>
#include <cctype>
#include <limits>

namespace
{
bool safeKeyToken(const std::string& value, std::size_t max)
{
    if (value.empty() || value.size() > max) return false;
    for (unsigned char c : value) {
        if (!std::isalnum(c) && c != '-' && c != '_' &&
            c != '.' && c != ':') return false;
    }
    return true;
}

bool sourceVideoMatchesEvidence(
    const MediaSourceDescriptor& source,
    const MediaPresentationProfile& plan,
    const MediaPlaybackPathEvidence& evidence)
{
    if (plan.videoAction != MediaTrackAction::Copy) return true;
    if (plan.sourceVideoStreamIndex < 0 ||
        static_cast<std::size_t>(plan.sourceVideoStreamIndex) >= source.videoStreams.size())
        return false;
    const auto& video = source.videoStreams[static_cast<std::size_t>(
        plan.sourceVideoStreamIndex)];
    if (video.codec != evidence.videoCodec) return false;

    // HEVC Main/Main10 cannot be inferred from the coarse H265 codec.
    // Until source ffprobe AND actual playback evidence supply matching
    // profile, bit depth and output transfer, never permit HEVC copy.
    if (video.codec == MediaCodec::H265) {
        if (video.codecProfile.empty() || evidence.videoProfile.empty() ||
            video.codecProfile != evidence.videoProfile ||
            video.bitDepth <= 0 || evidence.verifiedVideoBitDepth <= 0 ||
            video.bitDepth > evidence.verifiedVideoBitDepth)
            return false;
    }
    if (video.hdr) {
        if (!evidence.hdrOutputObserved ||
            video.colorTransfer.empty() || evidence.hdrTransfer.empty() ||
            video.colorTransfer != evidence.hdrTransfer)
            return false;
    }
    if (evidence.verifiedMaxFrameRate > 0.0 &&
        (video.framesPerSecond <= 0.0 ||
         video.framesPerSecond > evidence.verifiedMaxFrameRate))
        return false;
    return true;
}

int cost(const MediaPresentationProfile& p)
{
    if (p.adaptationClass == MediaAdaptationClass::PassThrough) return 0;
    if (p.videoAction == MediaTrackAction::Transcode) return 3;
    if (p.audioAction == MediaTrackAction::Transcode) return 2;
    return 1;
}
} // namespace

bool DeviceMediaProfileKey::valid() const
{
    return safeKeyToken(deviceId, 128) &&
        safeKeyToken(appVersion, 96) &&
        safeKeyToken(firmwareVersion, 96) &&
        safeKeyToken(playerVersion, 96);
}

bool DeviceMediaProfileKey::operator==(const DeviceMediaProfileKey& other) const
{
    return deviceId == other.deviceId && appVersion == other.appVersion &&
        firmwareVersion == other.firmwareVersion &&
        playerVersion == other.playerVersion && engine == other.engine;
}

DeviceMediaSelection DeviceMediaProfileSelector::select(
    const MediaSourceDescriptor& source,
    const ClientMediaCapabilities& advertised,
    const DeviceMediaCapabilityProfile& profile,
    const DeviceMediaProfileKey& authenticatedPlayer,
    int preferredAudioStreamIndex) const
{
    DeviceMediaSelection result;
    result.reason = "no_exact_verified_player_path";
    if (!authenticatedPlayer.valid() || profile.version !=
        DeviceMediaCapabilityProfile::ContractVersion ||
        !(profile.key == authenticatedPlayer) ||
        profile.verifiedPaths.size() > 32U) {
        result.reason = "device_player_profile_mismatch";
        return result;
    }

    int best = std::numeric_limits<int>::max();
    for (const auto& observed : profile.verifiedPaths) {
        if (observed.engine != authenticatedPlayer.engine) continue;
        const auto narrowed = narrowToVerifiedPath(
            advertised, observed, authenticatedPlayer.engine,
            observed.protocol, observed.container, observed.videoCodec,
            observed.audioCodec, false);
        if (!narrowed.available) continue;
        const MediaPresentationProfile candidate =
            MediaPresentationSelector().select(
                source, narrowed.capabilities, preferredAudioStreamIndex);
        if (!candidate.available || candidate.protocol != observed.protocol ||
            candidate.container != observed.container ||
            candidate.targetVideoCodec != observed.videoCodec ||
            candidate.targetAudioCodec != observed.audioCodec ||
            candidate.targetVideoWidth > observed.verifiedVideoWidth ||
            candidate.targetVideoHeight > observed.verifiedVideoHeight ||
            candidate.targetAudioChannels > observed.verifiedAudioChannels ||
            !sourceVideoMatchesEvidence(source, candidate, observed)) continue;
        const int currentCost = cost(candidate);
        if (currentCost < best) {
            best = currentCost;
            result.available = true;
            result.engine = observed.engine;
            result.presentation = candidate;
            result.reason = "exact_verified_path";
        }
    }
    return result;
}
