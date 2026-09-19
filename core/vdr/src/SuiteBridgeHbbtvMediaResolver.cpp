#include "SuiteBridgeHbbtvMediaResolver.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>

namespace
{
bool stringField(const std::string& json, const std::string& key, std::string& value)
{
    const std::string marker = "\"" + key + "\":\"";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) return false;
    std::size_t position = start + marker.size();
    value.clear();
    while (position < json.size()) {
        const char character = json[position++];
        if (character == '"') return true;
        if (character == '\\') {
            if (position >= json.size()) return false;
            const char escaped = json[position++];
            if (escaped == '"' || escaped == '\\' || escaped == '/')
                value.push_back(escaped);
            else return false;
        } else if (static_cast<unsigned char>(character) < 0x20U) {
            return false;
        } else {
            value.push_back(character);
        }
    }
    return false;
}

bool unsignedField(const std::string& json, const std::string& key, std::uint64_t& value)
{
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) return false;
    std::size_t position = start + marker.size();
    while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) ++position;
    if (position >= json.size() || !std::isdigit(static_cast<unsigned char>(json[position]))) return false;
    std::uint64_t parsed = 0;
    while (position < json.size() && std::isdigit(static_cast<unsigned char>(json[position]))) {
        const unsigned digit = static_cast<unsigned>(json[position] - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
        ++position;
    }
    value = parsed;
    return true;
}

bool boolField(const std::string& json, const std::string& key, bool& value)
{
    const std::string marker = "\"" + key + "\":";
    const std::size_t start = json.find(marker);
    if (start == std::string::npos) return false;
    const std::size_t position = start + marker.size();
    if (json.compare(position, 4, "true") == 0) { value = true; return true; }
    if (json.compare(position, 5, "false") == 0) { value = false; return true; }
    return false;
}

bool safeSocketPath(const std::string& value)
{
    if (value.empty() || value.front() != '/' || value.size() >= 108U ||
        value.find("..") != std::string::npos || value.find('?') != std::string::npos ||
        value.find('#') != std::string::npos) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '/' || c == '-' || c == '_' || c == '.' || c == ':';
    });
}

bool stateFromWire(const std::string& name, std::uint64_t code, HbbtvMediaSourceState& state)
{
    switch (code) {
        case 0: if (name != "none") return false; state = HbbtvMediaSourceState::None; return true;
        case 1: if (name != "streaming") return false; state = HbbtvMediaSourceState::Streaming; return true;
        case 2: if (name != "paused") return false; state = HbbtvMediaSourceState::Paused; return true;
        case 3: if (name != "stopped") return false; state = HbbtvMediaSourceState::Stopped; return true;
        case 4: if (name != "failed") return false; state = HbbtvMediaSourceState::Failed; return true;
    }
    return false;
}

bool resultConsistent(const std::string& name, std::uint64_t code)
{
    static const char* Names[] = {"ok", "invalid_request", "no_session", "session_mismatch"};
    return code < 4U && name == Names[code];
}
}

SuiteBridgeHbbtvMediaResolver::SuiteBridgeHbbtvMediaResolver(
    ISuiteBridgeHbbtvTransport& transport) : transport_(transport) {}

HbbtvMediaSource SuiteBridgeHbbtvMediaResolver::resolveMedia(const std::string& sessionId)
{
    HbbtvMediaSource result;
    if (sessionId.empty() || sessionId.size() >= 128U) {
        result.error = "hbbtv_media_request_invalid";
        return result;
    }

    SuiteBridgeHbbtvMediaRequest request;
    request.sessionId = sessionId;
    const SuiteBridgeHbbtvCommandReply reply = transport_.readHbbtvMedia(request);
    if (!reply.transportSucceeded || reply.replyCode != 250 || reply.payload.size() > 4096U) {
        result.error = "hbbtv_media_transport_failed";
        return result;
    }

    std::uint64_t schema = 0, providerSchema = 0, resultCode = 0, stateCode = 0;
    std::uint64_t revision = 0, x = 0, y = 0, width = 0, height = 0;
    std::string provider, capability, wireResult, wireSession, wireState, socketPath;
    bool fullscreen = true, connected = false;

    if (!unsignedField(reply.payload, "schemaVersion", schema) || schema != 1U ||
        !stringField(reply.payload, "provider", provider) || provider != "vdr-plugin-web" ||
        !unsignedField(reply.payload, "providerSchemaVersion", providerSchema) || providerSchema != 1U ||
        !stringField(reply.payload, "capability", capability) || capability != "broadcast.hbbtv.media" ||
        !stringField(reply.payload, "result", wireResult) ||
        !unsignedField(reply.payload, "resultCode", resultCode) || !resultConsistent(wireResult, resultCode) ||
        !stringField(reply.payload, "sessionId", wireSession) || wireSession != sessionId ||
        !stringField(reply.payload, "state", wireState) ||
        !unsignedField(reply.payload, "stateCode", stateCode) || !stateFromWire(wireState, stateCode, result.state) ||
        !unsignedField(reply.payload, "mediaRevision", revision) ||
        !boolField(reply.payload, "fullscreen", fullscreen) ||
        !boolField(reply.payload, "consumerConnected", connected) ||
        !unsignedField(reply.payload, "x", x) || !unsignedField(reply.payload, "y", y) ||
        !unsignedField(reply.payload, "width", width) || !unsignedField(reply.payload, "height", height) ||
        !stringField(reply.payload, "socketPath", socketPath) ||
        x > 16384U || y > 16384U || width > 16384U || height > 16384U) {
        result.error = "hbbtv_media_payload_invalid";
        return result;
    }

    if (resultCode == 2U) return result;
    if (resultCode != 0U) {
        result.error = "hbbtv_media_" + wireResult;
        return result;
    }

    result.mediaRevision = revision;
    result.fullscreen = fullscreen;
    result.consumerConnected = connected;
    result.x = static_cast<std::int32_t>(x);
    result.y = static_cast<std::int32_t>(y);
    result.width = static_cast<std::int32_t>(width);
    result.height = static_cast<std::int32_t>(height);

    if (result.state == HbbtvMediaSourceState::Streaming ||
        result.state == HbbtvMediaSourceState::Paused) {
        if (revision == 0 || !safeSocketPath(socketPath)) {
            result.error = "hbbtv_media_payload_invalid";
            return result;
        }
        result.unixSocketPath = socketPath;
        result.available = true;
        return result;
    }

    if (!socketPath.empty()) {
        result.error = "hbbtv_media_payload_invalid";
        return result;
    }
    return result;
}
