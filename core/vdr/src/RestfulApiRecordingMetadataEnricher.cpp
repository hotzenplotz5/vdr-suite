#include "RestfulApiRecordingMetadataEnricher.h"

#include "RestfulApiRecordingMetadataMapper.h"

#include <cctype>
#include <map>
#include <string>
#include <vector>

namespace
{

std::size_t skipWhitespace(
    const std::string& text,
    std::size_t position)
{
    while (position < text.size() &&
           std::isspace(static_cast<unsigned char>(text[position])))
    {
        ++position;
    }

    return position;
}

std::size_t findMatching(
    const std::string& text,
    const std::size_t start,
    const char openCharacter,
    const char closeCharacter)
{
    bool inString = false;
    bool escaped = false;
    int depth = 0;

    for (std::size_t index = start;
         index < text.size();
         ++index)
    {
        const char character = text[index];

        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (character == '\\')
            {
                escaped = true;
            }
            else if (character == '"')
            {
                inString = false;
            }

            continue;
        }

        if (character == '"')
        {
            inString = true;
            continue;
        }

        if (character == openCharacter)
        {
            ++depth;
        }
        else if (character == closeCharacter)
        {
            --depth;

            if (depth == 0)
            {
                return index;
            }
        }
    }

    return std::string::npos;
}

std::string extractRecordingsArray(
    const std::string& json)
{
    const std::size_t keyPosition =
        json.find("\"recordings\"");

    const std::size_t arrayStart = json.find(
        '[',
        keyPosition == std::string::npos
            ? 0
            : keyPosition);

    if (arrayStart == std::string::npos)
    {
        return {};
    }

    const std::size_t arrayEnd =
        findMatching(json, arrayStart, '[', ']');

    if (arrayEnd == std::string::npos)
    {
        return {};
    }

    return json.substr(
        arrayStart + 1,
        arrayEnd - arrayStart - 1);
}

std::vector<std::string> splitTopLevelObjects(
    const std::string& arrayText)
{
    std::vector<std::string> objects;
    std::size_t position = 0;

    while (position < arrayText.size())
    {
        const std::size_t objectStart =
            arrayText.find('{', position);

        if (objectStart == std::string::npos)
        {
            break;
        }

        const std::size_t objectEnd =
            findMatching(arrayText, objectStart, '{', '}');

        if (objectEnd == std::string::npos)
        {
            break;
        }

        objects.push_back(
            arrayText.substr(
                objectStart,
                objectEnd - objectStart + 1));
        position = objectEnd + 1;
    }

    return objects;
}

bool getIntegerField(
    const std::string& objectText,
    const std::string& fieldName,
    long long& value)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());

    if (colon == std::string::npos)
    {
        return false;
    }

    std::size_t position = skipWhitespace(objectText, colon + 1);

    if (position >= objectText.size())
    {
        return false;
    }

    bool negative = false;

    if (objectText[position] == '-')
    {
        negative = true;
        ++position;
    }

    if (position >= objectText.size() ||
        !std::isdigit(static_cast<unsigned char>(objectText[position])))
    {
        return false;
    }

    long long parsed = 0;

    while (position < objectText.size() &&
           std::isdigit(static_cast<unsigned char>(objectText[position])))
    {
        parsed = parsed * 10 + (objectText[position] - '0');
        ++position;
    }

    value = negative ? -parsed : parsed;
    return true;
}

}

void RestfulApiRecordingMetadataEnricher::enrich(
    const std::string& recordingsJson,
    std::vector<VdrRecording>& recordings)
{
    if (recordings.empty())
    {
        return;
    }

    std::map<std::string, VdrRecording*> recordingsById;

    for (VdrRecording& recording : recordings)
    {
        if (!recording.id.empty())
        {
            recordingsById[recording.id] = &recording;
        }
    }

    const std::string arrayText =
        extractRecordingsArray(recordingsJson);

    for (const std::string& objectText :
         splitTopLevelObjects(arrayText))
    {
        long long number = 0;

        if (!getIntegerField(objectText, "number", number) ||
            number < 0)
        {
            continue;
        }

        const auto match = recordingsById.find(
            std::to_string(number));

        if (match == recordingsById.end() ||
            match->second == nullptr)
        {
            continue;
        }

        match->second->metadata =
            RestfulApiRecordingMetadataMapper::mapRecordingObject(
                objectText);
    }
}
