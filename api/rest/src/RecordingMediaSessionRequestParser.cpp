#include "RecordingMediaSessionRequestParser.h"

#include <cctype>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

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
            switch (escaped) {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            default: return false;
            }
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

bool readObjectField(
    const std::string& object,
    const std::string& key,
    std::string& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) ||
        position >= object.size() || object[position] != '{') {
        return false;
    }

    const std::size_t start = position;
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (; position < object.size(); ++position) {
        const char character = object[position];
        if (inString) {
            if (escaped) escaped = false;
            else if (character == '\\') escaped = true;
            else if (character == '"') inString = false;
            continue;
        }
        if (character == '"') {
            inString = true;
            continue;
        }
        if (character == '{') ++depth;
        else if (character == '}') {
            --depth;
            if (depth == 0) {
                result = object.substr(start, position - start + 1);
                return true;
            }
            if (depth < 0) return false;
        }
    }
    return false;
}

bool readStringArrayField(
    const std::string& object,
    const std::string& key,
    std::vector<std::string>& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) ||
        position >= object.size() || object[position] != '[') {
        return false;
    }

    ++position;
    result.clear();
    for (;;) {
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') {
            ++position;
            return true;
        }

        std::string value;
        if (!readStringAt(object, position, value)) return false;
        result.push_back(std::move(value));
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') {
            ++position;
            return true;
        }
        if (object[position] != ',') return false;
        ++position;
    }
}

bool readBoolField(
    const std::string& object,
    const std::string& key,
    bool& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) return false;
    if (object.compare(position, 4, "true") == 0) {
        const std::size_t end = position + 4;
        if (end < object.size() &&
            !std::isspace(static_cast<unsigned char>(object[end])) &&
            object[end] != ',' && object[end] != '}') return false;
        result = true;
        return true;
    }
    if (object.compare(position, 5, "false") == 0) {
        const std::size_t end = position + 5;
        if (end < object.size() &&
            !std::isspace(static_cast<unsigned char>(object[end])) &&
            object[end] != ',' && object[end] != '}') return false;
        result = false;
        return true;
    }
    return false;
}

bool readNonNegativeIntField(
    const std::string& object,
    const std::string& key,
    int& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) return false;
    if (position >= object.size() ||
        !std::isdigit(static_cast<unsigned char>(object[position]))) {
        return false;
    }

    long value = 0;
    std::size_t cursor = position;
    while (cursor < object.size() &&
        std::isdigit(static_cast<unsigned char>(object[cursor]))) {
        const int digit = object[cursor] - '0';
        if (value > (16384 - digit) / 10) return false;
        value = value * 10 + digit;
        ++cursor;
    }

    if (cursor < object.size() &&
        !std::isspace(static_cast<unsigned char>(object[cursor])) &&
        object[cursor] != ',' && object[cursor] != '}') {
        return false;
    }

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

bool safeRecordingId(const std::string& value)
{
    if (value.empty() || value.size() > 512) return false;
    for (unsigned char character : value) {
        if (character < 0x20 || character == 0x7f) return false;
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

bool parseProtocols(
    const std::vector<std::string>& values,
    std::vector<MediaDeliveryProtocol>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "progressive") result.push_back(MediaDeliveryProtocol::Progressive);
        else if (value == "hls") result.push_back(MediaDeliveryProtocol::Hls);
        else return false;
    }
    return !result.empty();
}

bool parseContainers(
    const std::vector<std::string>& values,
    std::vector<MediaContainer>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "mpeg-ts") result.push_back(MediaContainer::MpegTs);
        else if (value == "mp4") result.push_back(MediaContainer::Mp4);
        else if (value == "fmp4") result.push_back(MediaContainer::Fmp4);
        else return false;
    }
    return !result.empty();
}

bool parseCodec(const std::string& value, MediaCodec& codec)
{
    if (value == "h264") codec = MediaCodec::H264;
    else if (value == "h265" || value == "hevc") codec = MediaCodec::H265;
    else if (value == "mpeg2video") codec = MediaCodec::Mpeg2Video;
    else if (value == "aac") codec = MediaCodec::Aac;
    else if (value == "ac3") codec = MediaCodec::Ac3;
    else if (value == "eac3") codec = MediaCodec::Eac3;
    else if (value == "mpeg-audio" || value == "mp2" || value == "mp3") codec = MediaCodec::MpegAudio;
    else return false;
    return true;
}

bool parseCodecs(
    const std::vector<std::string>& values,
    std::vector<MediaCodec>& result)
{
    result.clear();
    for (const auto& value : values) {
        MediaCodec codec = MediaCodec::Unknown;
        if (!parseCodec(value, codec)) return false;
        result.push_back(codec);
    }
    return true;
}

RecordingMediaSessionRequest invalid(const std::string& reasonCode)
{
    RecordingMediaSessionRequest result;
    result.reasonCode = reasonCode;
    return result;
}

RecordingMediaSessionStopRequest invalidStop(const std::string& reasonCode)
{
    RecordingMediaSessionStopRequest result;
    result.reasonCode = reasonCode;
    return result;
}

} // namespace

RecordingMediaSessionRequest RecordingMediaSessionRequestParser::parse(
    const std::string& body) const
{
    RecordingMediaSessionRequest request;
    if (!readStringField(body, "backendId", request.backendId) ||
        !safeBackendId(request.backendId)) {
        return invalid("invalid_backend_id");
    }
    if (!readStringField(body, "recordingId", request.recordingId) ||
        !safeRecordingId(request.recordingId)) {
        return invalid("invalid_recording_id");
    }

    std::string capabilitiesObject;
    if (!readObjectField(body, "capabilities", capabilitiesObject)) {
        return invalid("invalid_media_capabilities");
    }

    std::vector<std::string> protocols;
    std::vector<std::string> containers;
    std::vector<std::string> videoCodecs;
    std::vector<std::string> audioCodecs;
    if (!readStringArrayField(capabilitiesObject, "protocols", protocols) ||
        !parseProtocols(protocols, request.capabilities.protocols) ||
        !readStringArrayField(capabilitiesObject, "containers", containers) ||
        !parseContainers(containers, request.capabilities.containers) ||
        !readStringArrayField(capabilitiesObject, "videoCodecs", videoCodecs) ||
        !parseCodecs(videoCodecs, request.capabilities.videoCodecs) ||
        !readStringArrayField(capabilitiesObject, "audioCodecs", audioCodecs) ||
        !parseCodecs(audioCodecs, request.capabilities.audioCodecs) ||
        !readBoolField(
            capabilitiesObject,
            "supportsByteRanges",
            request.capabilities.supportsByteRanges) ||
        !readNonNegativeIntField(
            capabilitiesObject,
            "maxVideoWidth",
            request.capabilities.maxVideoWidth) ||
        !readNonNegativeIntField(
            capabilitiesObject,
            "maxVideoHeight",
            request.capabilities.maxVideoHeight)) {
        return invalid("invalid_media_capabilities");
    }

    request.valid = true;
    return request;
}

RecordingMediaSessionStopRequest RecordingMediaSessionRequestParser::parseStop(
    const std::string& body) const
{
    std::size_t operationPosition = 0;
    if (!locateValue(body, "operation", operationPosition)) {
        return invalidStop("media_session_stop_not_requested");
    }

    std::string operation;
    if (!readStringAt(body, operationPosition, operation) || operation != "stop") {
        return invalidStop("invalid_media_session_operation");
    }

    RecordingMediaSessionStopRequest request;
    if (!readStringField(body, "backendId", request.backendId) ||
        !safeBackendId(request.backendId)) {
        return invalidStop("invalid_backend_id");
    }
    if (!readStringField(body, "sessionId", request.sessionId) ||
        !safeSessionId(request.sessionId)) {
        return invalidStop("invalid_media_session_id");
    }

    request.valid = true;
    return request;
}