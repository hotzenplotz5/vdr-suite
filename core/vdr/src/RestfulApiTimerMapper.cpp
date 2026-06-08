#include "RestfulApiTimerMapper.h"

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
    std::string result;
    bool escaped = false;

    for (char c : value) {
        if (escaped) {
            switch (c) {
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case '"':
            case '\\':
            case '/':
                result.push_back(c);
                break;
            default:
                result.push_back(c);
                break;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

std::string getStringField(const std::string& objectText, const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return "";
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
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
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
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
    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return fallback;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
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

    return fallback;
}

std::string extractTimersArray(const std::string& json)
{
    std::size_t timersKey = json.find("\"timers\"");
    if (timersKey != std::string::npos) {
        std::size_t arrayStart = json.find('[', timersKey);
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

VdrTimer mapObjectToTimer(const std::string& objectText)
{
    VdrTimer timer;

    timer.id = getStringField(objectText, "id");
    if (timer.id.empty()) {
        int number = getIntField(objectText, "number", -1);
        if (number >= 0) {
            timer.id = std::to_string(number);
        }
    }

    timer.channelId = getStringField(objectText, "channel_id");
    if (timer.channelId.empty()) {
        timer.channelId = getStringField(objectText, "channelid");
    }

    timer.eventId = getStringField(objectText, "event_id");
    if (timer.eventId.empty()) {
        timer.eventId = getStringField(objectText, "eventid");
    }

    timer.title = getStringField(objectText, "title");
    if (timer.title.empty()) {
        timer.title = getStringField(objectText, "file");
    }

    timer.subtitle = getStringField(objectText, "short_text");
    if (timer.subtitle.empty()) {
        timer.subtitle = getStringField(objectText, "aux");
    }

    timer.startTime = std::to_string(getIntField(objectText, "start_time", getIntField(objectText, "starttime", 0)));
    timer.endTime = std::to_string(getIntField(objectText, "stop_time", getIntField(objectText, "stoptime", 0)));

    timer.priority = getIntField(objectText, "priority", 0);
    timer.lifetime = getIntField(objectText, "lifetime", 0);

    timer.enabled = getBoolField(objectText, "active", true);
    timer.recording = getBoolField(objectText, "recording", false);

    return timer;
}

}

std::vector<VdrTimer> RestfulApiTimerMapper::parseTimers(const std::string& json)
{
    std::vector<VdrTimer> timers;

    std::string arrayText = extractTimersArray(json);
    if (arrayText.empty()) {
        return timers;
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        VdrTimer timer = mapObjectToTimer(objectText);
        if (!timer.id.empty()) {
            timers.push_back(timer);
        }
    }

    return timers;
}
