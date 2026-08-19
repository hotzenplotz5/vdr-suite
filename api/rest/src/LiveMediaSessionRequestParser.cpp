#include "LiveMediaSessionRequestParser.h"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr int MaximumTypedAudioChannels = 32;

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
        if (static_cast<unsigned char>(character) < 0x20U) return false;
        result.push_back(character);
    }
    return false;
}

bool readStringField(const std::string& object, const std::string& key, std::string& result)
{
    std::size_t position = 0;
    return locateValue(object, key, position) && readStringAt(object, position, result);
}

bool readOptionalStringField(
    const std::string& object,
    const std::string& key,
    std::string& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) {
        result.clear();
        return true;
    }
    return readStringAt(object, position, result);
}

bool readObjectField(const std::string& object, const std::string& key, std::string& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) ||
        position >= object.size() || object[position] != '{') return false;
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
        if (character == '"') { inString = true; continue; }
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
        position >= object.size() || object[position] != '[') return false;
    ++position;
    result.clear();
    for (;;) {
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') return true;
        std::string value;
        if (!readStringAt(object, position, value)) return false;
        result.push_back(std::move(value));
        skipWhitespace(object, position);
        if (position >= object.size()) return false;
        if (object[position] == ']') return true;
        if (object[position] != ',') return false;
        ++position;
    }
}

bool readBoolField(const std::string& object, const std::string& key, bool& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position)) return false;
    if (object.compare(position, 4, "true") == 0) { result = true; return true; }
    if (object.compare(position, 5, "false") == 0) { result = false; return true; }
    return false;
}

bool readNonNegativeIntField(const std::string& object, const std::string& key, int& result)
{
    std::size_t position = 0;
    if (!locateValue(object, key, position) || position >= object.size() ||
        !std::isdigit(static_cast<unsigned char>(object[position]))) return false;
    int value = 0;
    while (position < object.size() &&
           std::isdigit(static_cast<unsigned char>(object[position]))) {
        const int digit = object[position++] - '0';
        if (value > (16384 - digit) / 10) return false;
        value = value * 10 + digit;
    }
    result = value;
    return true;
}

bool safeToken(const std::string& value, std::size_t maximum)
{
    if (value.empty() || value.size() > maximum) return false;
    for (unsigned char character : value)
        if (!std::isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') return false;
    return true;
}

bool parseProtocols(const std::vector<std::string>& values,
                    std::vector<MediaDeliveryProtocol>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "hls") result.push_back(MediaDeliveryProtocol::Hls);
        else if (value == "progressive") result.push_back(MediaDeliveryProtocol::Progressive);
        else return false;
    }
    return !result.empty();
}

bool parseContainers(const std::vector<std::string>& values,
                     std::vector<MediaContainer>& result)
{
    result.clear();
    for (const auto& value : values) {
        if (value == "fmp4") result.push_back(MediaContainer::Fmp4);
        else if (value == "mpeg-ts") result.push_back(MediaContainer::MpegTs);
        else if (value == "mp4") result.push_back(MediaContainer::Mp4);
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

bool parseCodecs(const std::vector<std::string>& values, std::vector<MediaCodec>& result)
{
    result.clear();
    for (const auto& value : values) {
        MediaCodec codec = MediaCodec::Unknown;
        if (!parseCodec(value, codec)) return false;
        result.push_back(codec);
    }
    return true;
}

LiveMediaSessionRequest invalid(const std::string& reason)
{
    LiveMediaSessionRequest result;
    result.reasonCode = reason;
    return result;
}
}

bool LiveMediaSessionRequestParser::requestsLiveChannel(const std::string& body)
{
    std::string resourceKind;
    return readStringField(body, "resourceKind", resourceKind) &&
        resourceKind == "live-channel";
}

LiveMediaSessionRequest LiveMediaSessionRequestParser::parse(const std::string& body) const
{
    LiveMediaSessionRequest request;
    std::string resourceKind;
    if (!readStringField(body, "resourceKind", resourceKind) ||
        resourceKind != "live-channel") return invalid("invalid_live_resource_kind");
    if (!readStringField(body, "backendId", request.backendId) ||
        !safeToken(request.backendId, 128)) return invalid("invalid_backend_id");
    if (!readStringField(body, "channelId", request.channelId) ||
        !safeToken(request.channelId, 128)) return invalid("invalid_channel_id");
    if (!readOptionalStringField(body, "replacesSessionId", request.replacesSessionId) ||
        (!request.replacesSessionId.empty() && !safeToken(request.replacesSessionId, 128)))
        return invalid("invalid_replaces_session_id");

    std::string capabilitiesObject;
    if (!readObjectField(body, "capabilities", capabilitiesObject))
        return invalid("invalid_media_capabilities");
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
        !readBoolField(capabilitiesObject, "supportsByteRanges", request.capabilities.supportsByteRanges) ||
        !readNonNegativeIntField(capabilitiesObject, "maxVideoWidth", request.capabilities.maxVideoWidth) ||
        !readNonNegativeIntField(capabilitiesObject, "maxVideoHeight", request.capabilities.maxVideoHeight) ||
        !readNonNegativeIntField(capabilitiesObject, "maxAudioChannels", request.capabilities.maxAudioChannels) ||
        request.capabilities.maxAudioChannels > MaximumTypedAudioChannels) {
        return invalid("invalid_media_capabilities");
    }
    request.valid = true;
    return request;
}
