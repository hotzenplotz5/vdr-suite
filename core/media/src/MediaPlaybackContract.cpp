#include "MediaPlaybackContract.h"

#include <algorithm>

namespace
{

std::string jsonEscape(const std::string& value)
{
    std::string result;
    result.reserve(value.size());
    for (unsigned char character : value) {
        switch (character) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (character < 0x20U) {
                static constexpr char Hex[] = "0123456789abcdef";
                result += "\\u00";
                result.push_back(Hex[(character >> 4) & 0x0f]);
                result.push_back(Hex[character & 0x0f]);
            }
            else {
                result.push_back(static_cast<char>(character));
            }
        }
    }
    return result;
}

std::string nullableString(const std::string& value)
{
    return value.empty() ? "null" : "\"" + jsonEscape(value) + "\"";
}

const char* resourceModeName(MediaPlaybackResourceMode mode)
{
    switch (mode) {
    case MediaPlaybackResourceMode::CompletedRecording: return "recording";
    case MediaPlaybackResourceMode::GrowingRecording: return "growing-recording";
    case MediaPlaybackResourceMode::Live: return "live";
    }
    return "recording";
}

const char* seekModeName(MediaPlaybackSeekMode mode)
{
    switch (mode) {
    case MediaPlaybackSeekMode::Unsupported: return "unsupported";
    case MediaPlaybackSeekMode::InSessionReposition: return "in-session-reposition";
    case MediaPlaybackSeekMode::ReplacementSessionRestart: return "replacement-session-restart";
    }
    return "unsupported";
}

bool hlsProfile(const std::string& profileId)
{
    return profileId == "hls-fmp4" || profileId == "hls-ts";
}

std::string booleanJson(bool value)
{
    return value ? "true" : "false";
}

} // namespace

MediaPlaybackContract MediaPlaybackContractFactory::recording(
    const std::string& presentationProfileId,
    bool growing,
    int positionSeconds,
    int durationSeconds,
    int presentationBasePositionSeconds,
    bool indexedTimelineReady,
    bool indexPreparing,
    const MediaPlaybackTrackCapabilities& tracks)
{
    MediaPlaybackContract contract;
    contract.resourceMode = growing
        ? MediaPlaybackResourceMode::GrowingRecording
        : MediaPlaybackResourceMode::CompletedRecording;
    contract.presentationProfileId = presentationProfileId;
    contract.positionSeconds = std::max(0, positionSeconds);
    contract.durationSeconds = growing ? 0 : std::max(0, durationSeconds);
    contract.presentationBasePositionSeconds = std::max(0, presentationBasePositionSeconds);
    contract.tracks = tracks;

    const bool completedTimelineReady =
        !growing && contract.durationSeconds > 0 && indexedTimelineReady;
    const bool progressiveFmp4 = presentationProfileId == "progressive-fmp4";
    const bool hls = hlsProfile(presentationProfileId);
    const bool restartProfile = progressiveFmp4 || hls;

    contract.restartSupported = restartProfile && completedTimelineReady;
    contract.restartPreparing =
        restartProfile && !growing && !completedTimelineReady && indexPreparing;

    if (progressiveFmp4) {
        contract.seek.mode = MediaPlaybackSeekMode::InSessionReposition;
        contract.seek.supported = completedTimelineReady;
        contract.seek.preparing =
            !growing && !completedTimelineReady && indexPreparing;
    }
    else if (hls) {
        contract.seek.mode = MediaPlaybackSeekMode::ReplacementSessionRestart;
        contract.seek.supported = completedTimelineReady;
        contract.seek.preparing =
            !growing && !completedTimelineReady && indexPreparing;
    }

    if (contract.seek.supported) {
        contract.seek.windowStartSeconds = 0;
        contract.seek.windowEndSeconds = contract.durationSeconds;
    }

    return contract;
}

MediaPlaybackContract MediaPlaybackContractFactory::live(
    const std::string& presentationProfileId,
    const MediaPlaybackTrackCapabilities& tracks)
{
    MediaPlaybackContract contract;
    contract.resourceMode = MediaPlaybackResourceMode::Live;
    contract.presentationProfileId = presentationProfileId;
    contract.tracks = tracks;
    return contract;
}

std::string MediaPlaybackContractFactory::json(const MediaPlaybackContract& contract)
{
    std::string result =
        "{\"contractVersion\":" + std::to_string(contract.contractVersion) +
        ",\"resourceMode\":\"" + std::string(resourceModeName(contract.resourceMode)) + "\"" +
        ",\"presentationProfileId\":" + nullableString(contract.presentationProfileId) +
        ",\"playback\":{"
            "\"positionSeconds\":" + std::to_string(std::max(0, contract.positionSeconds)) +
            ",\"durationSeconds\":";
    if (contract.durationSeconds > 0) result += std::to_string(contract.durationSeconds);
    else result += "null";
    result +=
        ",\"presentationBasePositionSeconds\":" +
            std::to_string(std::max(0, contract.presentationBasePositionSeconds)) +
        ",\"pauseSupported\":" + booleanJson(contract.pauseSupported) +
        ",\"resumeSupported\":" + booleanJson(contract.resumePlaybackSupported) +
        ",\"restart\":{"
            "\"supported\":" + booleanJson(contract.restartSupported) +
            ",\"preparing\":" + booleanJson(contract.restartPreparing) +
        "}}";

    result +=
        ",\"seek\":{"
            "\"supported\":" + booleanJson(contract.seek.supported) +
            ",\"mode\":\"" + std::string(seekModeName(contract.seek.mode)) + "\"" +
            ",\"preparing\":" + booleanJson(contract.seek.preparing);
    if (contract.seek.supported &&
        contract.seek.windowEndSeconds > contract.seek.windowStartSeconds) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                std::to_string(std::max(0, contract.seek.windowStartSeconds)) +
            ",\"endSeconds\":" +
                std::to_string(std::max(0, contract.seek.windowEndSeconds)) + "}";
    }
    result += "}";

    result +=
        ",\"tracks\":{"
            "\"audioSelection\":{\"supported\":" +
                booleanJson(contract.tracks.audioSelectionSupported) + "}," +
            "\"subtitleSelection\":{\"supported\":" +
                booleanJson(contract.tracks.subtitleSelectionSupported) + "}," +
            "\"subtitleOff\":{\"supported\":" +
                booleanJson(contract.tracks.subtitleOffSupported) + "}}";

    result += " ,\"continuity\":{";
    result += "\"generation\":";
    if (contract.continuityGeneration > 0) {
        result += std::to_string(contract.continuityGeneration);
    }
    else {
        result += "null";
    }
    result +=
        ",\"state\":" + nullableString(contract.continuityState) + "}";

    result += ",\"failure\":";
    if (contract.failureClass.empty()) {
        result += "null";
    }
    else {
        result +=
            "{\"class\":\"" + jsonEscape(contract.failureClass) + "\"," +
            "\"reasonCode\":" + nullableString(contract.failureReasonCode) + "}";
    }
    result += "}";
    return result;
}

std::string MediaPlaybackContractFactory::legacyPlaybackJson(
    const MediaPlaybackContract& contract)
{
    const bool inSessionSeek =
        contract.seek.mode == MediaPlaybackSeekMode::InSessionReposition;
    const bool seekSupported = inSessionSeek && contract.seek.supported;
    const bool seekPreparing = inSessionSeek && contract.seek.preparing;

    std::string result =
        "{\"positionSeconds\":" + std::to_string(std::max(0, contract.positionSeconds)) +
        ",\"durationSeconds\":";
    if (contract.durationSeconds > 0) result += std::to_string(contract.durationSeconds);
    else result += "null";

    result +=
        ",\"seek\":{\"supported\":" + booleanJson(seekSupported) +
        ",\"preparing\":" + booleanJson(seekPreparing);
    if (seekSupported &&
        contract.seek.windowEndSeconds > contract.seek.windowStartSeconds) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                std::to_string(std::max(0, contract.seek.windowStartSeconds)) +
            ",\"endSeconds\":" +
                std::to_string(std::max(0, contract.seek.windowEndSeconds)) + "}";
    }
    result +=
        "},\"resume\":{\"supported\":" + booleanJson(contract.restartSupported) +
        ",\"preparing\":" + booleanJson(contract.restartPreparing) + "}}";
    return result;
}
