#include "RestfulApiRecordingMapper.h"

#include <algorithm>
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

std::string normalizeRecordingName(const std::string& name)
{
    std::string normalized = name;

    std::replace(
        normalized.begin(),
        normalized.end(),
        '~',
        '/');

    return normalized;
}

std::string normalizePersonName(const std::string& name)
{
    std::string normalized;
    bool previousWasSeparator = false;

    for (char c : name) {
        unsigned char uc = static_cast<unsigned char>(c);

        if (std::isalnum(uc)) {
            normalized.push_back(
                static_cast<char>(std::tolower(uc)));
            previousWasSeparator = false;
        } else if (!previousWasSeparator && !normalized.empty()) {
            normalized.push_back('-');
            previousWasSeparator = true;
        }
    }

    while (!normalized.empty() && normalized.back() == '-') {
        normalized.pop_back();
    }

    return normalized;
}

std::string getObjectField(
    const std::string& objectText,
    const std::string& fieldName)
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

    std::size_t objectStart = objectText.find('{', colon + 1);
    if (objectStart == std::string::npos) {
        return "";
    }

    std::size_t objectEnd = findMatching(objectText, objectStart, '{', '}');
    if (objectEnd == std::string::npos) {
        return "";
    }

    return objectText.substr(objectStart, objectEnd - objectStart + 1);
}

std::string getArrayField(
    const std::string& objectText,
    const std::string& fieldName)
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

PersonCollection parseAdditionalMediaActors(
    const std::string& objectText)
{
    PersonCollection persons =
        PersonCollection::createEmpty();

    const std::string additionalMedia =
        getObjectField(objectText, "additional_media");

    if (additionalMedia.empty()) {
        return persons;
    }

    const std::string actorsArray =
        getArrayField(additionalMedia, "actors");

    if (actorsArray.empty()) {
        return persons;
    }

    const std::vector<std::string> actorObjects =
        splitTopLevelObjects(actorsArray);

    for (const std::string& actorObject : actorObjects) {
        const std::string name =
            getStringField(actorObject, "name");

        if (name.empty()) {
            continue;
        }

        const std::string characterName =
            getStringField(actorObject, "role");

        persons.add(
            Person::withCharacterName(
                ContentClassificationSource::Tvscraper,
                PersonRole::Actor,
                name,
                normalizePersonName(name),
                characterName));
    }

    return persons;
}

long long getLongLongField(const std::string& objectText, const std::string& fieldName, long long fallback = 0)
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

    long long value = 0;
    while (pos < objectText.size() && std::isdigit(static_cast<unsigned char>(objectText[pos]))) {
        value = value * 10 + (objectText[pos] - '0');
        ++pos;
    }

    return negative ? -value : value;
}

std::string extractRecordingsArray(const std::string& json)
{
    std::size_t recordingsKey = json.find("\"recordings\"");
    if (recordingsKey != std::string::npos) {
        std::size_t arrayStart = json.find('[', recordingsKey);
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

VdrRecording mapObjectToRecording(const std::string& objectText)
{
    VdrRecording recording;

    int number = getIntField(objectText, "number", -1);
    std::string fileName = getStringField(objectText, "file_name");
    std::string relativePath = getStringField(objectText, "relative_file_name");

    recording.id = number >= 0 ? std::to_string(number) : "";
    recording.title = normalizeRecordingName(getStringField(objectText, "name"));
    recording.path = relativePath.empty() ? fileName : relativePath;
    recording.backendNativeId = fileName;
    recording.startTime = std::to_string(getIntField(objectText, "event_start_time", 0));
    recording.durationSeconds = getIntField(objectText, "duration", 0);
    recording.sizeMb = getLongLongField(objectText, "filesize_mb", 0);
    recording.persons = parseAdditionalMediaActors(objectText);

    return recording;
}

}

std::vector<VdrRecording> RestfulApiRecordingMapper::parseRecordings(const std::string& json)
{
    std::vector<VdrRecording> recordings;

    std::string arrayText = extractRecordingsArray(json);
    if (arrayText.empty()) {
        return recordings;
    }

    std::vector<std::string> objects = splitTopLevelObjects(arrayText);
    for (const std::string& objectText : objects) {
        VdrRecording recording = mapObjectToRecording(objectText);
        if (!recording.id.empty()) {
            recordings.push_back(recording);
        }
    }

    return recordings;
}
