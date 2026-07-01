#include "RestfulApiChannelMapper.h"

#include "JsonStringDecoder.h"

#include <cctype>
#include <string>
#include <vector>

namespace {

std::size_t skipWhitespace(const std::string& text, std::size_t pos)
{
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
        ++pos;
    }
    return pos;
}

std::size_t findMatching(const std::string& text, std::size_t start, char openChar, char closeChar)
{
    bool inString = false;
    bool escaped = false;
    int depth = 0;

    for (std::size_t i = start; i < text.size(); ++i) {
        char c = text[i];

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            continue;
        }

        if (c == openChar) {
            ++depth;
        } else if (c == closeChar) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }

    return std::string::npos;
}

std::vector<std::string> splitTopLevelObjects(const std::string& arrayText)
{
    std::vector<std::string> objects;
    std::size_t pos = 0;

    while (pos < arrayText.size()) {
        std::size_t start = arrayText.find('{', pos);
        if (start == std::string::npos) {
            break;
        }

        std::size_t end = findMatching(arrayText, start, '{', '}');
        if (end == std::string::npos) {
            break;
        }

        objects.push_back(arrayText.substr(start, end - start + 1));
        pos = end + 1;
    }

    return objects;
}

std::string unescapeJsonString(const std::string& value)
{
    return vdrsuite::decodeJsonStringEscapes(value);
}

std::string toLowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (char character : value) {
        lowered.push_back(
            static_cast<char>(
                std::tolower(static_cast<unsigned char>(character))));
    }

    return lowered;
}

std::size_t findFieldColon(const std::string& objectText, const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return std::string::npos;
    }

    return objectText.find(':', keyPos + key.size());
}

bool hasField(const std::string& objectText, const std::string& fieldName)
{
    return findFieldColon(objectText, fieldName) != std::string::npos;
}

std::string getStringField(const std::string& objectText, const std::string& fieldName)
{
    std::size_t colon = findFieldColon(objectText, fieldName);
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t quoteStart = objectText.find('"', colon + 1);
    if (quoteStart == std::string::npos) {
        return "";
    }

    bool escaped = false;
    for (std::size_t i = quoteStart + 1; i < objectText.size(); ++i) {
        char c = objectText[i];

        if (escaped) {
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            return unescapeJsonString(objectText.substr(quoteStart + 1, i - quoteStart - 1));
        }
    }

    return "";
}

int getIntField(const std::string& objectText, const std::string& fieldName, int fallback = 0)
{
    std::size_t colon = findFieldColon(objectText, fieldName);
    if (colon == std::string::npos) {
        return fallback;
    }

    std::size_t pos = skipWhitespace(objectText, colon + 1);
    if (pos >= objectText.size()) {
        return fallback;
    }

    bool negative = false;
    if (objectText[pos] == '-') {
        negative = true;
        ++pos;
    }

    if (pos >= objectText.size() || !std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        return fallback;
    }

    int value = 0;
    while (pos < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        value = value * 10 + (objectText[pos] - '0');
        ++pos;
    }

    return negative ? -value : value;
}

bool getBoolField(const std::string& objectText, const std::string& fieldName, bool fallback = false)
{
    std::size_t colon = findFieldColon(objectText, fieldName);
    if (colon == std::string::npos) {
        return fallback;
    }

    std::size_t pos = skipWhitespace(objectText, colon + 1);
    if (objectText.compare(pos, 4, "true") == 0) {
        return true;
    }

    if (objectText.compare(pos, 5, "false") == 0) {
        return false;
    }

    if (objectText.compare(pos, 1, "1") == 0) {
        return true;
    }

    if (objectText.compare(pos, 1, "0") == 0) {
        return false;
    }

    if (pos < objectText.size() && objectText[pos] == '"') {
        const std::string value = toLowerAscii(getStringField(objectText, fieldName));
        if (value == "true" || value == "yes" || value == "1") {
            return true;
        }
        if (value == "false" || value == "no" || value == "0") {
            return false;
        }
    }

    return fallback;
}

bool getFirstBoolField(
    const std::string& objectText,
    const std::vector<std::string>& fieldNames,
    bool fallback)
{
    for (const std::string& fieldName : fieldNames) {
        if (hasField(objectText, fieldName)) {
            return getBoolField(objectText, fieldName, fallback);
        }
    }

    return fallback;
}

std::string getArrayFieldText(const std::string& objectText, const std::string& fieldName)
{
    std::size_t colon = findFieldColon(objectText, fieldName);
    if (colon == std::string::npos) {
        return "";
    }

    std::size_t arrayStart = objectText.find('[', colon + 1);
    if (arrayStart == std::string::npos) {
        return "";
    }

    std::size_t arrayEnd = findMatching(objectText, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return "";
    }

    return objectText.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
}

bool tokenMeansNonZeroCaid(const std::string& token)
{
    std::string cleaned;

    for (char character : token) {
        if (!std::isspace(static_cast<unsigned char>(character)) &&
            character != '"' &&
            character != '\'') {
            cleaned.push_back(character);
        }
    }

    if (cleaned.empty() || cleaned == "0") {
        return false;
    }

    for (char character : cleaned) {
        if (character >= '1' && character <= '9') {
            return true;
        }
    }

    return false;
}

bool hasNonZeroCaids(const std::string& objectText)
{
    for (const std::string& fieldName : {"caids", "CAIDs", "caid", "CAID"}) {
        if (!hasField(objectText, fieldName)) {
            continue;
        }

        const std::string arrayText = getArrayFieldText(objectText, fieldName);
        if (!arrayText.empty()) {
            std::size_t start = 0;
            while (start <= arrayText.size()) {
                std::size_t end = arrayText.find(',', start);
                const std::string token = arrayText.substr(
                    start,
                    end == std::string::npos
                        ? std::string::npos
                        : end - start);

                if (tokenMeansNonZeroCaid(token)) {
                    return true;
                }

                if (end == std::string::npos) {
                    break;
                }
                start = end + 1;
            }
        }

        const int numericCaid = getIntField(objectText, fieldName, 0);
        if (numericCaid > 0) {
            return true;
        }

        const std::string stringCaid = getStringField(objectText, fieldName);
        if (tokenMeansNonZeroCaid(stringCaid)) {
            return true;
        }
    }

    return false;
}

std::string extractChannelsArray(const std::string& json)
{
    std::size_t channelsKey = json.find("\"channels\"");
    if (channelsKey != std::string::npos) {
        std::size_t arrayStart = json.find('[', channelsKey);
        if (arrayStart == std::string::npos) {
            return "";
        }

        std::size_t arrayEnd = findMatching(json, arrayStart, '[', ']');
        if (arrayEnd == std::string::npos) {
            return "";
        }

        return json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    }

    std::size_t arrayStart = json.find('[');
    if (arrayStart == std::string::npos) {
        return "";
    }

    std::size_t arrayEnd = findMatching(json, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return "";
    }

    return json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
}

VdrChannel mapObjectToChannel(const std::string& objectText)
{
    VdrChannel channel;

    channel.id = getStringField(objectText, "channel_id");
    channel.number = getIntField(objectText, "number", 0);
    channel.name = getStringField(objectText, "name");
    channel.provider = getStringField(objectText, "provider");
    channel.group = getStringField(objectText, "group");
    channel.radio = getFirstBoolField(objectText, {"is_radio", "radio"}, false);
    channel.encrypted = getFirstBoolField(
        objectText,
        {"encrypted", "is_encrypted", "scrambled", "is_scrambled"},
        hasNonZeroCaids(objectText));
    channel.enabled = getFirstBoolField(objectText, {"enabled", "is_enabled", "active"}, true);

    return channel;
}

}

std::vector<VdrChannel> RestfulApiChannelMapper::parseChannels(const std::string& json)
{
    std::vector<VdrChannel> channels;

    std::string arrayText = extractChannelsArray(json);
    if (arrayText.empty()) {
        return channels;
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        VdrChannel channel = mapObjectToChannel(objectText);
        if (!channel.id.empty()) {
            channels.push_back(channel);
        }
    }

    return channels;
}
