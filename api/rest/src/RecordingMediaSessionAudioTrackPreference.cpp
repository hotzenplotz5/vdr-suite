#include "RecordingMediaSessionAudioTrackPreference.h"

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
            if (escaped == '"' || escaped == '\\' || escaped == '/') {
                result.push_back(escaped);
            }
            else {
                return false;
            }
            continue;
        }
        if (static_cast<unsigned char>(character) < 0x20) return false;
        result.push_back(character);
    }
    return false;
}

bool safeAudioTrackId(const std::string& value)
{
    if (value.size() < 7 || value.size() > 16 || value.rfind("audio-", 0) != 0) {
        return false;
    }
    if (value[6] == '0') return false;
    for (std::size_t index = 6; index < value.size(); ++index) {
        if (!std::isdigit(static_cast<unsigned char>(value[index]))) return false;
    }
    return true;
}

} // namespace

RecordingMediaSessionAudioTrackPreference
RecordingMediaSessionAudioTrackPreferenceParser::parse(
    const std::string& body) const
{
    RecordingMediaSessionAudioTrackPreference result;
    std::size_t position = 0;
    if (!locateValue(body, "audioTrackId", position)) {
        result.valid = true;
        return result;
    }

    if (!readStringAt(body, position, result.audioTrackId) ||
        !safeAudioTrackId(result.audioTrackId)) {
        result.reasonCode = "invalid_audio_track_id";
        return result;
    }

    result.valid = true;
    return result;
}
