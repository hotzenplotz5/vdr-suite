#include "RestfulApiEventMapper.h"

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

std::vector<std::string> getStringArrayField(const std::string& objectText, const std::string& fieldName)
{
    std::vector<std::string> values;

    const std::string key = "\"" + fieldName + "\"";
    std::size_t keyPos = objectText.find(key);
    if (keyPos == std::string::npos) {
        return values;
    }

    std::size_t colon = objectText.find(':', keyPos + key.size());
    if (colon == std::string::npos) {
        return values;
    }

    std::size_t arrayStart = objectText.find('[', colon + 1);
    if (arrayStart == std::string::npos) {
        return values;
    }

    std::size_t arrayEnd = findMatching(objectText, arrayStart, '[', ']');
    if (arrayEnd == std::string::npos) {
        return values;
    }

    std::string arrayText = objectText.substr(arrayStart + 1, arrayEnd - arrayStart - 1);
    std::size_t pos = 0;

    while (pos < arrayText.size()) {
        std::size_t quoteStart = arrayText.find('"', pos);
        if (quoteStart == std::string::npos) {
            break;
        }

        bool escaped = false;
        for (std::size_t i = quoteStart + 1; i < arrayText.size(); ++i) {
            char c = arrayText[i];

            if (escaped) {
                escaped = false;
                continue;
            }

            if (c == '\\') {
                escaped = true;
                continue;
            }

            if (c == '"') {
                values.push_back(unescapeJsonString(arrayText.substr(quoteStart + 1, i - quoteStart - 1)));
                pos = i + 1;
                break;
            }
        }

        if (quoteStart == arrayText.find('"', pos)) {
            break;
        }
    }

    return values;
}

std::string extractEventsArray(const std::string& json)
{
    std::size_t eventsKey = json.find("\"events\"");
    if (eventsKey != std::string::npos) {
        std::size_t arrayStart = json.find('[', eventsKey);
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

VdrEvent mapObjectToEvent(const std::string& objectText)
{
    VdrEvent event;

    int id = getIntField(objectText, "id", 0);
    int startTime = getIntField(objectText, "start_time", 0);
    int duration = getIntField(objectText, "duration", 0);

    event.id = id > 0 ? std::to_string(id) : "";
    event.channelId = getStringField(objectText, "channel");
    event.title = getStringField(objectText, "title");
    event.subtitle = getStringField(objectText, "short_text");
    event.description = getStringField(objectText, "description");
    event.startTime = startTime > 0 ? std::to_string(startTime) : "";
    event.endTime = startTime > 0 ? std::to_string(startTime + duration) : "";
    event.durationSeconds = duration;
    event.contentDescriptors = getStringArrayField(objectText, "contents");
    event.parentalRating = getIntField(objectText, "parental_rating", 0);

    return event;
}

}

std::vector<VdrEvent> RestfulApiEventMapper::parseEvents(const std::string& json)
{
    std::vector<VdrEvent> events;

    std::string arrayText = extractEventsArray(json);
    if (arrayText.empty()) {
        return events;
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        VdrEvent event = mapObjectToEvent(objectText);
        if (!event.id.empty()) {
            events.push_back(event);
        }
    }

    return events;
}
