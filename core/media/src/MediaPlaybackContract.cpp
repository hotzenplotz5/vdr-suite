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
    case MediaPlaybackResourceMode::Recording: return "recording";
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

std::string optionalBoolJson(const std::optional<bool>& value)
{
    if (!value.has_value()) return "null";
    return *value ? "true" : "false";
}

std::string optionalIntJson(const std::optional<int>& value)
{
    if (!value.has_value()) return "null";
    return std::to_string(std::max(0, *value));
}

std::string optionalStringJson(const std::optional<std::string>& value)
{
    if (!value.has_value()) return "null";
    return nullableString(*value);
}

bool optionalTrue(const std::optional<bool>& value)
{
    return value.has_value() && *value;
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
    const bool completedTimelineReady =
        !growing && durationSeconds > 0 && indexedTimelineReady;
    const bool progressiveFmp4 = presentationProfileId == "progressive-fmp4";
    const bool hls = hlsProfile(presentationProfileId);
    const bool restartProfile = progressiveFmp4 || hls;
    const bool restartSupported = restartProfile && completedTimelineReady;
    const bool restartPreparing =
        restartProfile && !growing && !completedTimelineReady && indexPreparing;
    const bool legacySeekSupported = progressiveFmp4 && completedTimelineReady;
    const bool legacySeekPreparing =
        progressiveFmp4 && !growing && !completedTimelineReady && indexPreparing;

    return recordingFromLegacy(
        presentationProfileId,
        growing,
        positionSeconds,
        growing || durationSeconds <= 0
            ? std::optional<int>{}
            : std::optional<int>{durationSeconds},
        presentationBasePositionSeconds,
        legacySeekSupported,
        legacySeekPreparing,
        restartSupported,
        restartPreparing,
        tracks);
}

MediaPlaybackContract MediaPlaybackContractFactory::recordingFromLegacy(
    const std::string& presentationProfileId,
    std::optional<bool> growing,
    std::optional<int> positionSeconds,
    std::optional<int> durationSeconds,
    std::optional<int> presentationBasePositionSeconds,
    std::optional<bool> legacySeekSupported,
    std::optional<bool> legacySeekPreparing,
    std::optional<bool> restartSupported,
    std::optional<bool> restartPreparing,
    const MediaPlaybackTrackCapabilities& tracks)
{
    MediaPlaybackContract contract;
    contract.resourceMode = growing.has_value() && *growing
        ? MediaPlaybackResourceMode::GrowingRecording
        : MediaPlaybackResourceMode::Recording;
    contract.presentationProfileId = presentationProfileId;
    contract.positionSeconds = positionSeconds;
    contract.durationSeconds = growing.has_value() && *growing
        ? std::optional<int>{}
        : durationSeconds;
    contract.presentationBasePositionSeconds = presentationBasePositionSeconds;
    contract.pauseSupported = true;
    contract.resumePlaybackSupported = true;
    contract.restartSupported = restartSupported;
    contract.restartPreparing = restartPreparing;
    contract.tracks = tracks;

    if (presentationProfileId == "progressive-fmp4") {
        contract.seek.mode = MediaPlaybackSeekMode::InSessionReposition;
        contract.seek.supported = legacySeekSupported;
        contract.seek.preparing = legacySeekPreparing;
    }
    else if (hlsProfile(presentationProfileId)) {
        contract.seek.mode = MediaPlaybackSeekMode::ReplacementSessionRestart;
        contract.seek.supported = restartSupported;
        contract.seek.preparing = restartPreparing;
    }
    else {
        contract.seek.mode = MediaPlaybackSeekMode::Unsupported;
        contract.seek.supported = false;
        contract.seek.preparing = false;
    }

    if (optionalTrue(contract.seek.supported) &&
        contract.durationSeconds.has_value() && *contract.durationSeconds > 0) {
        contract.seek.windowStartSeconds = 0;
        contract.seek.windowEndSeconds = *contract.durationSeconds;
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
    contract.seek.mode = MediaPlaybackSeekMode::Unsupported;
    contract.seek.supported = false;
    contract.seek.preparing = false;
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
            "\"positionSeconds\":" + optionalIntJson(contract.positionSeconds) +
            ",\"durationSeconds\":" + optionalIntJson(contract.durationSeconds) +
            ",\"presentationBasePositionSeconds\":" +
                optionalIntJson(contract.presentationBasePositionSeconds) +
            ",\"pauseSupported\":" + optionalBoolJson(contract.pauseSupported) +
            ",\"resumeSupported\":" + optionalBoolJson(contract.resumePlaybackSupported) +
            ",\"restart\":{"
                "\"supported\":" + optionalBoolJson(contract.restartSupported) +
                ",\"preparing\":" + optionalBoolJson(contract.restartPreparing) +
            "}}";

    result +=
        ",\"seek\":{"
            "\"supported\":" + optionalBoolJson(contract.seek.supported) +
            ",\"mode\":\"" + std::string(seekModeName(contract.seek.mode)) + "\"" +
            ",\"preparing\":" + optionalBoolJson(contract.seek.preparing);
    if (contract.seek.windowStartSeconds.has_value() &&
        contract.seek.windowEndSeconds.has_value() &&
        *contract.seek.windowEndSeconds > *contract.seek.windowStartSeconds) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                optionalIntJson(contract.seek.windowStartSeconds) +
            ",\"endSeconds\":" +
                optionalIntJson(contract.seek.windowEndSeconds) + "}";
    }
    result += "}";

    result +=
        ",\"tracks\":{"
            "\"audioSelection\":{\"supported\":" +
                optionalBoolJson(contract.tracks.audioSelectionSupported) + "}," +
            "\"subtitleSelection\":{\"supported\":" +
                optionalBoolJson(contract.tracks.subtitleSelectionSupported) + "}," +
            "\"subtitleOff\":{\"supported\":" +
                optionalBoolJson(contract.tracks.subtitleOffSupported) + "}}";

    result +=
        ",\"continuity\":{"
            "\"generation\":" + optionalIntJson(contract.continuityGeneration) +
            ",\"state\":" + optionalStringJson(contract.continuityState) + "}";

    result += ",\"failure\":";
    if (!contract.failureClass.has_value()) {
        result += "null";
    }
    else {
        result +=
            "{\"class\":" + optionalStringJson(contract.failureClass) + "," +
            "\"reasonCode\":" + optionalStringJson(contract.failureReasonCode) + "}";
    }
    result += "}";
    return result;
}

std::string MediaPlaybackContractFactory::legacyPlaybackJson(
    const MediaPlaybackContract& contract)
{
    const bool inSessionSeek =
        contract.seek.mode == MediaPlaybackSeekMode::InSessionReposition;
    const std::optional<bool> seekSupported = inSessionSeek
        ? contract.seek.supported
        : std::optional<bool>{false};
    const std::optional<bool> seekPreparing = inSessionSeek
        ? contract.seek.preparing
        : std::optional<bool>{false};

    std::string result =
        "{\"positionSeconds\":" + optionalIntJson(contract.positionSeconds) +
        ",\"durationSeconds\":" + optionalIntJson(contract.durationSeconds) +
        ",\"seek\":{\"supported\":" + optionalBoolJson(seekSupported) +
        ",\"preparing\":" + optionalBoolJson(seekPreparing);
    if (inSessionSeek && optionalTrue(seekSupported) &&
        contract.seek.windowStartSeconds.has_value() &&
        contract.seek.windowEndSeconds.has_value()) {
        result +=
            ",\"window\":{\"startSeconds\":" +
                optionalIntJson(contract.seek.windowStartSeconds) +
            ",\"endSeconds\":" +
                optionalIntJson(contract.seek.windowEndSeconds) + "}";
    }
    result +=
        "},\"resume\":{\"supported\":" + optionalBoolJson(contract.restartSupported) +
        ",\"preparing\":" + optionalBoolJson(contract.restartPreparing) + "}}";
    return result;
}
