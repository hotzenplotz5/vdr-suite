#include "RecordingMediaSessionStartPosition.h"

#include <cctype>
#include <limits>

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

} // namespace

RecordingMediaSessionStartPosition
RecordingMediaSessionStartPositionParser::parse(
    const std::string& body) const
{
    RecordingMediaSessionStartPosition result;
    std::size_t position = 0;
    if (!locateValue(body, "startPositionSeconds", position)) {
        result.valid = true;
        return result;
    }

    result.present = true;
    if (position >= body.size() ||
        !std::isdigit(static_cast<unsigned char>(body[position]))) {
        result.reasonCode = "invalid_recording_start_position";
        return result;
    }

    long long value = 0;
    std::size_t cursor = position;
    constexpr long long Maximum = std::numeric_limits<int>::max();
    while (cursor < body.size() &&
        std::isdigit(static_cast<unsigned char>(body[cursor]))) {
        const int digit = body[cursor] - '0';
        if (value > (Maximum - digit) / 10) {
            result.reasonCode = "invalid_recording_start_position";
            return result;
        }
        value = value * 10 + digit;
        ++cursor;
    }

    skipWhitespace(body, cursor);
    if (cursor >= body.size() ||
        (body[cursor] != ',' && body[cursor] != '}')) {
        result.reasonCode = "invalid_recording_start_position";
        return result;
    }

    result.seconds = static_cast<int>(value);
    result.valid = true;
    return result;
}
