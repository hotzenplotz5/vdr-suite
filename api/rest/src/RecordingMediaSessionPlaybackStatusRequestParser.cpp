#include "RecordingMediaSessionRequestParser.h"

#include <cctype>
#include <string>

namespace
{

void skipWhitespace(const std::string& value, std::size_t& position)
{
    while (position < value.size() &&
        std::isspace(static_cast<unsigned char>(value[position]))) {
        ++position;
    }
}

bool locateValue(
    const std::string& object,
    const std::string& key,
    std::size_t& position)
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

bool readStringAt(
    const std::string& object,
    std::size_t& position,
    std::string& result)
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
            if (escaped == '"' || escaped == '\\' || escaped == '/')
                result.push_back(escaped);
            else
                return false;
            continue;
        }
        if (static_cast<unsigned char>(character) < 0x20) return false;
        result.push_back(character);
    }
    return false;
}

bool readStringField(
    const std::string& object,
    const std::string& key,
    std::string& result)
{
    std::size_t position = 0;
    return locateValue(object, key, position) &&
        readStringAt(object, position, result);
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

RecordingMediaSessionPlaybackStatusRequest invalidStatus(
    const std::string& reasonCode)
{
    RecordingMediaSessionPlaybackStatusRequest result;
    result.reasonCode = reasonCode;
    return result;
}

} // namespace

RecordingMediaSessionPlaybackStatusRequest
RecordingMediaSessionRequestParser::parsePlaybackStatus(
    const std::string& body) const
{
    std::size_t operationPosition = 0;
    if (!locateValue(body, "operation", operationPosition)) {
        return invalidStatus("media_session_playback_status_not_requested");
    }

    std::string operation;
    if (!readStringAt(body, operationPosition, operation)) {
        return invalidStatus("invalid_media_session_operation");
    }
    if (operation != "playback-status") {
        if (operation == "stop" || operation == "seek") {
            return invalidStatus("media_session_playback_status_not_requested");
        }
        return invalidStatus("invalid_media_session_operation");
    }

    RecordingMediaSessionPlaybackStatusRequest request;
    if (!readStringField(body, "backendId", request.backendId) ||
        !safeBackendId(request.backendId)) {
        return invalidStatus("invalid_backend_id");
    }
    if (!readStringField(body, "sessionId", request.sessionId) ||
        !safeSessionId(request.sessionId)) {
        return invalidStatus("invalid_media_session_id");
    }

    request.valid = true;
    return request;
}
