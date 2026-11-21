#include "RecordingMediaSessionRequestParser.h"

#include <cctype>
#include <limits>
#include <string>

namespace
{

void skipWhitespace(const std::string& value, std::size_t& position)
{
    while (position < value.size() &&
        std::isspace(static_cast<unsigned char>(value[position]))) ++position;
}

bool locateValue(const std::string& object, const std::string& key, std::size_t& position)
{
    const std::string token = "\"" + key + "\"";
    std::size_t cursor = 0;
    while ((cursor = object.find(token, cursor)) != std::string::npos) {
        std::size_t colon = cursor + token.size();
        skipWhitespace(object, colon);
        if (colon < object.size() && object[colon] == ':') {
            position = colon + 1;
            skipWhitespace(object, position);
            return true;
        }
        cursor += token.size();
    }
    return false;
}

bool readStringAt(const std::string& object, std::size_t& position, std::string& result)
{
    if (position >= object.size() || object[position] != '"') return false;
    ++position;
    result.clear();
    while (position < object.size()) {
        const char character = object[position++];
        if (character == '"') return true;
        if (character == '\\') {
            if (position >= object.size()) return false;
            const char escaped = object[position++];
            if (escaped == '"' || escaped == '\\' || escaped == '/') result.push_back(escaped);
            else return false;
            continue;
        }
        if (static_cast<unsigned char>(character) < 0x20) return false;
        result.push_back(character);
    }
    return false;
}

bool readStringField(const std::string& object, const std::string& key, std::string& result)
{
    std::size_t position = 0;
    return locateValue(object, key, position) && readStringAt(object, position, result);
}

bool readNonNegativeIntField(const std::string& object, const std::string& key, int& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) || position >= object.size() ||
        !std::isdigit(static_cast<unsigned char>(object[position]))) return false;
    long long value = 0;
    std::size_t cursor = position;
    while (cursor < object.size() && std::isdigit(static_cast<unsigned char>(object[cursor]))) {
        const int digit = object[cursor] - '0';
        if (value > (static_cast<long long>(std::numeric_limits<int>::max()) - digit) / 10) return false;
        value = value * 10 + digit;
        ++cursor;
    }
    if (cursor < object.size() && !std::isspace(static_cast<unsigned char>(object[cursor])) &&
        object[cursor] != ',' && object[cursor] != '}') return false;
    result = static_cast<int>(value);
    return true;
}

bool safeBackendId(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    for (unsigned char character : value) {
        if (!std::isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') return false;
    }
    return true;
}

bool safeSessionId(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    for (unsigned char character : value) {
        if (!std::isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') return false;
    }
    return true;
}

bool safeNormalizedTrackId(const std::string& value, const std::string& prefix, std::size_t maximumLength)
{
    if (value.size() <= prefix.size() || value.size() > maximumLength ||
        value.rfind(prefix, 0) != 0 || value[prefix.size()] == '0') return false;
    for (std::size_t index = prefix.size(); index < value.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(value[index]))) return false;
    }
    return true;
}

bool safeAudioTrackId(const std::string& value)
{
    return safeNormalizedTrackId(value, "audio-", 16);
}

bool safeSubtitleTrackId(const std::string& value)
{
    return value == "off" || safeNormalizedTrackId(value, "subtitle-", 20);
}

bool knownDifferentOperation(const std::string& operationName)
{
    return operationName == "stop" || operationName == "seek" ||
        operationName == "playback-status" || operationName == "track-status" ||
        operationName == "select-audio-track" || operationName == "select-subtitle-track";
}

RecordingMediaSessionTrackStatusRequest invalidTrackStatus(const std::string& reasonCode)
{
    RecordingMediaSessionTrackStatusRequest result;
    result.reasonCode = reasonCode;
    return result;
}

RecordingMediaSessionAudioTrackSelectionRequest invalidAudioSelection(const std::string& reasonCode)
{
    RecordingMediaSessionAudioTrackSelectionRequest result;
    result.reasonCode = reasonCode;
    return result;
}

RecordingMediaSessionSubtitleTrackSelectionRequest invalidSubtitleSelection(const std::string& reasonCode)
{
    RecordingMediaSessionSubtitleTrackSelectionRequest result;
    result.reasonCode = reasonCode;
    return result;
}

} // namespace

RecordingMediaSessionTrackStatusRequest RecordingMediaSessionRequestParser::parseTrackStatus(
    const std::string& body) const
{
    std::size_t operationPosition = 0;
    if (!locateValue(body, "operation", operationPosition))
        return invalidTrackStatus("media_session_track_status_not_requested");
    std::string operationName;
    if (!readStringAt(body, operationPosition, operationName))
        return invalidTrackStatus("invalid_media_session_operation");
    if (operationName != "track-status")
        return invalidTrackStatus(knownDifferentOperation(operationName)
            ? "media_session_track_status_not_requested" : "invalid_media_session_operation");

    RecordingMediaSessionTrackStatusRequest request;
    if (!readStringField(body, "backendId", request.backendId) || !safeBackendId(request.backendId))
        return invalidTrackStatus("invalid_backend_id");
    if (!readStringField(body, "sessionId", request.sessionId) || !safeSessionId(request.sessionId))
        return invalidTrackStatus("invalid_media_session_id");
    request.valid = true;
    return request;
}

RecordingMediaSessionAudioTrackSelectionRequest RecordingMediaSessionRequestParser::parseAudioTrackSelection(
    const std::string& body) const
{
    std::size_t operationPosition = 0;
    if (!locateValue(body, "operation", operationPosition))
        return invalidAudioSelection("media_session_audio_track_selection_not_requested");
    std::string operationName;
    if (!readStringAt(body, operationPosition, operationName))
        return invalidAudioSelection("invalid_media_session_operation");
    if (operationName != "select-audio-track")
        return invalidAudioSelection(knownDifferentOperation(operationName)
            ? "media_session_audio_track_selection_not_requested" : "invalid_media_session_operation");

    RecordingMediaSessionAudioTrackSelectionRequest request;
    if (!readStringField(body, "backendId", request.backendId) || !safeBackendId(request.backendId))
        return invalidAudioSelection("invalid_backend_id");
    if (!readStringField(body, "sessionId", request.sessionId) || !safeSessionId(request.sessionId))
        return invalidAudioSelection("invalid_media_session_id");
    if (!readStringField(body, "audioTrackId", request.audioTrackId) || !safeAudioTrackId(request.audioTrackId))
        return invalidAudioSelection("invalid_audio_track_id");
    if (!readNonNegativeIntField(body, "positionSeconds", request.positionSeconds))
        return invalidAudioSelection("invalid_recording_audio_track_position");

    const RecordingMediaSessionRequest mediaRequest = parse(body);
    if (!mediaRequest.valid)
        return invalidAudioSelection(mediaRequest.reasonCode.empty() ? "invalid_media_capabilities" : mediaRequest.reasonCode);
    request.recordingId = mediaRequest.recordingId;
    request.capabilities = mediaRequest.capabilities;
    request.valid = true;
    return request;
}

RecordingMediaSessionSubtitleTrackSelectionRequest RecordingMediaSessionRequestParser::parseSubtitleTrackSelection(
    const std::string& body) const
{
    std::size_t operationPosition = 0;
    if (!locateValue(body, "operation", operationPosition))
        return invalidSubtitleSelection("media_session_subtitle_track_selection_not_requested");
    std::string operationName;
    if (!readStringAt(body, operationPosition, operationName))
        return invalidSubtitleSelection("invalid_media_session_operation");
    if (operationName != "select-subtitle-track")
        return invalidSubtitleSelection(knownDifferentOperation(operationName)
            ? "media_session_subtitle_track_selection_not_requested" : "invalid_media_session_operation");

    RecordingMediaSessionSubtitleTrackSelectionRequest request;
    if (!readStringField(body, "backendId", request.backendId) || !safeBackendId(request.backendId))
        return invalidSubtitleSelection("invalid_backend_id");
    if (!readStringField(body, "sessionId", request.sessionId) || !safeSessionId(request.sessionId))
        return invalidSubtitleSelection("invalid_media_session_id");
    if (!readStringField(body, "subtitleTrackId", request.subtitleTrackId) || !safeSubtitleTrackId(request.subtitleTrackId))
        return invalidSubtitleSelection("invalid_subtitle_track_id");
    if (!readNonNegativeIntField(body, "streamBasePositionSeconds", request.streamBasePositionSeconds))
        return invalidSubtitleSelection("invalid_recording_subtitle_stream_base");
    request.valid = true;
    return request;
}
