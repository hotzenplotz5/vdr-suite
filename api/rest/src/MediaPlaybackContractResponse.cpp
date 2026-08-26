#include "MediaPlaybackContractResponse.h"

#include "MediaPlaybackContract.h"

#include <cctype>
#include <optional>
#include <string>

namespace
{

struct Span
{
    std::size_t begin = 0;
    std::size_t end = 0;
};

std::optional<std::size_t> matchingBrace(
    const std::string& json,
    std::size_t open,
    std::size_t limit)
{
    if (open >= limit || json[open] != '{') return std::nullopt;
    int depth = 0;
    bool quoted = false;
    bool escaped = false;
    for (std::size_t i = open; i < limit; ++i) {
        const char c = json[i];
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
            continue;
        }
        if (c == '"') quoted = true;
        else if (c == '{') ++depth;
        else if (c == '}' && --depth == 0) return i;
    }
    return std::nullopt;
}

std::optional<Span> objectField(
    const std::string& json,
    const std::string& key,
    std::size_t begin,
    std::size_t end)
{
    const std::string token = "\"" + key + "\":{";
    const std::size_t found = json.find(token, begin);
    if (found == std::string::npos) return std::nullopt;
    const std::size_t open = found + token.size() - 1;
    if (open >= end) return std::nullopt;
    const auto close = matchingBrace(json, open, end);
    if (!close.has_value()) return std::nullopt;
    return Span{open, *close + 1};
}

std::optional<std::string> stringField(
    const std::string& json,
    const std::string& key,
    const Span& scope)
{
    const std::string token = "\"" + key + "\":\"";
    const std::size_t found = json.find(token, scope.begin);
    if (found == std::string::npos || found >= scope.end) return std::nullopt;
    const std::size_t start = found + token.size();
    const std::size_t close = json.find('"', start);
    if (close == std::string::npos || close >= scope.end) return std::nullopt;
    return json.substr(start, close - start);
}

std::optional<bool> boolField(
    const std::string& json,
    const std::string& key,
    const Span& scope)
{
    const std::string token = "\"" + key + "\":";
    const std::size_t found = json.find(token, scope.begin);
    if (found == std::string::npos || found >= scope.end) return std::nullopt;
    const std::size_t start = found + token.size();
    if (json.compare(start, 4, "true") == 0) return true;
    if (json.compare(start, 5, "false") == 0) return false;
    return std::nullopt;
}

std::optional<int> intField(
    const std::string& json,
    const std::string& key,
    const Span& scope)
{
    const std::string token = "\"" + key + "\":";
    const std::size_t found = json.find(token, scope.begin);
    if (found == std::string::npos || found >= scope.end) return std::nullopt;
    std::size_t pos = found + token.size();
    if (json.compare(pos, 4, "null") == 0 || pos >= scope.end || !std::isdigit(static_cast<unsigned char>(json[pos]))) {
        return std::nullopt;
    }
    int value = 0;
    while (pos < scope.end && std::isdigit(static_cast<unsigned char>(json[pos]))) {
        value = value * 10 + static_cast<int>(json[pos] - '0');
        ++pos;
    }
    return value;
}

MediaPlaybackTrackCapabilities tracksFrom(
    const std::string& json,
    const Span& mediaSession)
{
    MediaPlaybackTrackCapabilities tracks;
    const auto root = objectField(json, "tracks", mediaSession.begin, mediaSession.end);
    if (!root.has_value()) return tracks;
    const auto audio = objectField(json, "audio", root->begin, root->end);
    if (audio.has_value()) {
        tracks.audioSelectionSupported = boolField(json, "selectionSupported", *audio);
    }
    const auto subtitles = objectField(json, "subtitles", root->begin, root->end);
    if (subtitles.has_value()) {
        tracks.subtitleSelectionSupported = boolField(json, "selectionSupported", *subtitles);
        tracks.subtitleOffSupported = boolField(json, "offSupported", *subtitles);
    }
    return tracks;
}

} // namespace

ApiResponse MediaPlaybackContractResponse::augment(ApiResponse response, bool liveResource)
{
    if (response.statusCode < 200 || response.statusCode >= 300 ||
        response.contentType.rfind("application/json", 0) != 0) {
        return response;
    }
    const auto mediaSession = objectField(response.body, "mediaSession", 0, response.body.size());
    if (!mediaSession.has_value() ||
        response.body.find("\"playbackContract\":", mediaSession->begin) < mediaSession->end) {
        return response;
    }
    const auto state = stringField(response.body, "state", *mediaSession);
    const auto profile = stringField(response.body, "presentationProfileId", *mediaSession);
    if (!state.has_value() || *state != "ready" || !profile.has_value() || profile->empty()) {
        return response;
    }

    const MediaPlaybackTrackCapabilities tracks = tracksFrom(response.body, *mediaSession);
    MediaPlaybackContract contract;
    if (liveResource) {
        contract = MediaPlaybackContractFactory::live(*profile, tracks);
    }
    else {
        const auto growing = boolField(response.body, "growing", *mediaSession);
        const auto playback = objectField(response.body, "playback", mediaSession->begin, mediaSession->end);
        std::optional<int> position;
        std::optional<int> duration;
        std::optional<bool> seekSupported;
        std::optional<bool> seekPreparing;
        std::optional<bool> restartSupported;
        std::optional<bool> restartPreparing;
        if (playback.has_value()) {
            position = intField(response.body, "positionSeconds", *playback);
            duration = intField(response.body, "durationSeconds", *playback);
            const auto seek = objectField(response.body, "seek", playback->begin, playback->end);
            if (seek.has_value()) {
                seekSupported = boolField(response.body, "supported", *seek);
                seekPreparing = boolField(response.body, "preparing", *seek);
            }
            const auto restart = objectField(response.body, "resume", playback->begin, playback->end);
            if (restart.has_value()) {
                restartSupported = boolField(response.body, "supported", *restart);
                restartPreparing = boolField(response.body, "preparing", *restart);
            }
        }
        contract = MediaPlaybackContractFactory::recordingFromLegacy(
            *profile,
            growing,
            position,
            duration,
            position,
            seekSupported,
            seekPreparing,
            restartSupported,
            restartPreparing,
            tracks);
    }

    response.body.insert(
        mediaSession->end - 1,
        ",\"playbackContract\":" + MediaPlaybackContractFactory::json(contract));
    return response;
}
