#include "RestfulApiRecordingMetadataEnricher.h"

#include "JsonStringDecoder.h"
#include "RestfulApiRecordingMetadataMapper.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
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

std::string getStringField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return {};
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());

    if (colon == std::string::npos)
    {
        return {};
    }

    const std::size_t quoteStart =
        skipWhitespace(objectText, colon + 1);

    if (quoteStart >= objectText.size() ||
        objectText[quoteStart] != '"')
    {
        return {};
    }

    bool escaped = false;

    for (std::size_t index = quoteStart + 1;
         index < objectText.size();
         ++index)
    {
        const char character = objectText[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            return vdrsuite::decodeJsonStringEscapes(
                objectText.substr(
                    quoteStart + 1,
                    index - quoteStart - 1));
        }
    }

    return {};
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

    if (objectText[position] == '"')
    {
        ++position;
    }

    bool negative = false;

    if (position < objectText.size() &&
        objectText[position] == '-')
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

long long integerField(
    const std::string& objectText,
    const std::string& fieldName,
    const long long fallback)
{
    long long value = fallback;
    return getIntegerField(objectText, fieldName, value)
        ? value
        : fallback;
}

std::string getObjectField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return {};
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());
    const std::size_t objectStart =
        colon == std::string::npos
            ? std::string::npos
            : objectText.find('{', colon + 1);

    if (objectStart == std::string::npos)
    {
        return {};
    }

    const std::size_t objectEnd =
        findMatching(objectText, objectStart, '{', '}');

    if (objectEnd == std::string::npos)
    {
        return {};
    }

    return objectText.substr(
        objectStart,
        objectEnd - objectStart + 1);
}

std::string getArrayField(
    const std::string& objectText,
    const std::string& fieldName)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return {};
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());
    const std::size_t arrayStart =
        colon == std::string::npos
            ? std::string::npos
            : objectText.find('[', colon + 1);

    if (arrayStart == std::string::npos)
    {
        return {};
    }

    const std::size_t arrayEnd =
        findMatching(objectText, arrayStart, '[', ']');

    if (arrayEnd == std::string::npos)
    {
        return {};
    }

    return objectText.substr(
        arrayStart + 1,
        arrayEnd - arrayStart - 1);
}

std::string normalizePersonName(
    const std::string& name)
{
    std::string normalized;
    bool previousWasSeparator = false;

    for (const char character : name)
    {
        const unsigned char value =
            static_cast<unsigned char>(character);

        if (std::isalnum(value))
        {
            normalized.push_back(
                static_cast<char>(std::tolower(value)));
            previousWasSeparator = false;
        }
        else if (!previousWasSeparator &&
                 !normalized.empty())
        {
            normalized.push_back('-');
            previousWasSeparator = true;
        }
    }

    while (!normalized.empty() &&
           normalized.back() == '-')
    {
        normalized.pop_back();
    }

    return normalized;
}

long long deriveRecordingStartTimeFromPath(
    std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');

    while (!path.empty() && path.back() == '/')
    {
        path.pop_back();
    }

    const std::size_t separator = path.rfind('/');
    const std::string segment =
        separator == std::string::npos
            ? path
            : path.substr(separator + 1);

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;

    if (std::sscanf(
            segment.c_str(),
            "%4d-%2d-%2d.%2d.%2d",
            &year,
            &month,
            &day,
            &hour,
            &minute) != 5)
    {
        return -1;
    }

    std::tm parts {};
    parts.tm_year = year - 1900;
    parts.tm_mon = month - 1;
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_isdst = -1;

    const std::time_t timestamp = std::mktime(&parts);
    return timestamp > 0
        ? static_cast<long long>(timestamp)
        : -1;
}

std::string lastPathSegment(
    std::string path)
{
    std::replace(path.begin(), path.end(), '\\', '/');

    while (!path.empty() && path.back() == '/')
    {
        path.pop_back();
    }

    const std::size_t separator = path.rfind('/');
    return separator == std::string::npos
        ? path
        : path.substr(separator + 1);
}

std::string recordingAliasKey(
    const VdrRecording& recording)
{
    const std::string path =
        recording.backendNativeId.empty()
            ? recording.path
            : recording.backendNativeId;
    const std::string leaf = lastPathSegment(path);

    if (leaf.empty() ||
        recording.startTime.empty())
    {
        return {};
    }

    return leaf + "|" +
        recording.startTime + "|" +
        std::to_string(recording.durationSeconds) + "|" +
        std::to_string(recording.sizeMb);
}

std::string recordingAliasKey(
    const std::string& objectText)
{
    std::string path =
        getStringField(objectText, "file_name");

    if (path.empty())
    {
        path = getStringField(
            objectText,
            "relative_file_name");
    }

    long long startTime =
        integerField(objectText, "event_start_time", -1);

    if (startTime <= 0)
    {
        startTime = deriveRecordingStartTimeFromPath(path);
    }

    long long duration =
        integerField(objectText, "duration", -1);

    if (duration <= 0)
    {
        duration = integerField(
            objectText,
            "event_duration",
            -1);
    }

    const long long sizeMb =
        integerField(objectText, "filesize_mb", 0);
    const std::string leaf = lastPathSegment(path);

    if (leaf.empty() || startTime <= 0)
    {
        return {};
    }

    return leaf + "|" +
        std::to_string(startTime) + "|" +
        std::to_string(duration) + "|" +
        std::to_string(sizeMb);
}

void mergeString(
    std::string& target,
    const std::string& source)
{
    if (target.empty() && !source.empty())
    {
        target = source;
    }
}

void mergeMetadata(
    VdrRecordingMetadata& target,
    const VdrRecordingMetadata& source)
{
    mergeString(
        target.native.eventTitle,
        source.native.eventTitle);
    mergeString(
        target.native.shortText,
        source.native.shortText);
    mergeString(
        target.native.description,
        source.native.description);

    VdrRecordingProviderMetadata& targetProvider =
        target.provider;
    const VdrRecordingProviderMetadata& sourceProvider =
        source.provider;

    if (!targetProvider.hasData() &&
        sourceProvider.hasData())
    {
        targetProvider = sourceProvider;
    }
    else
    {
        if (targetProvider.source == VdrRecordingMetadataSource::None &&
            sourceProvider.source != VdrRecordingMetadataSource::None)
        {
            targetProvider.source = sourceProvider.source;
        }

        if (targetProvider.contentKind == VdrRecordingContentKind::Unknown &&
            sourceProvider.contentKind != VdrRecordingContentKind::Unknown)
        {
            targetProvider.contentKind = sourceProvider.contentKind;
        }

        mergeString(targetProvider.movieId, sourceProvider.movieId);
        mergeString(targetProvider.seriesId, sourceProvider.seriesId);
        mergeString(targetProvider.episodeId, sourceProvider.episodeId);
        mergeString(targetProvider.title, sourceProvider.title);
        mergeString(targetProvider.originalTitle, sourceProvider.originalTitle);
        mergeString(targetProvider.tagline, sourceProvider.tagline);
        mergeString(targetProvider.overview, sourceProvider.overview);
        mergeString(targetProvider.genreText, sourceProvider.genreText);
        mergeString(targetProvider.releaseDate, sourceProvider.releaseDate);
        mergeString(targetProvider.seriesTitle, sourceProvider.seriesTitle);
        mergeString(targetProvider.episodeTitle, sourceProvider.episodeTitle);

        if (targetProvider.seasonNumber <= 0)
        {
            targetProvider.seasonNumber = sourceProvider.seasonNumber;
        }
        if (targetProvider.episodeNumber <= 0)
        {
            targetProvider.episodeNumber = sourceProvider.episodeNumber;
        }
        if (targetProvider.runtimeMinutes <= 0)
        {
            targetProvider.runtimeMinutes = sourceProvider.runtimeMinutes;
        }
        if (targetProvider.rating <= 0.0)
        {
            targetProvider.rating = sourceProvider.rating;
        }
    }

    for (const VdrRecordingArtworkRef& sourceArtwork :
         source.artwork)
    {
        const auto duplicate = std::find_if(
            target.artwork.begin(),
            target.artwork.end(),
            [&sourceArtwork](const VdrRecordingArtworkRef& targetArtwork)
            {
                return targetArtwork.kind == sourceArtwork.kind &&
                    targetArtwork.source == sourceArtwork.source &&
                    targetArtwork.reference == sourceArtwork.reference;
            });

        if (duplicate == target.artwork.end())
        {
            target.artwork.push_back(sourceArtwork);
        }
    }
}

bool samePerson(
    const Person& left,
    const Person& right)
{
    return left.source() == right.source() &&
        left.role() == right.role() &&
        left.originalName() == right.originalName() &&
        left.normalizedName() == right.normalizedName() &&
        left.characterName() == right.characterName();
}

void mergePerson(
    PersonCollection& target,
    const Person& person)
{
    const auto duplicate = std::find_if(
        target.all().begin(),
        target.all().end(),
        [&person](const Person& existing)
        {
            return samePerson(existing, person);
        });

    if (duplicate == target.all().end())
    {
        target.add(person);
    }
}

void mergeAdditionalMediaActors(
    const std::string& objectText,
    PersonCollection& persons)
{
    const std::string additionalMedia =
        getObjectField(objectText, "additional_media");
    const std::string actors =
        getArrayField(additionalMedia, "actors");

    for (const std::string& actorObject :
         splitTopLevelObjects(actors))
    {
        const std::string name =
            getStringField(actorObject, "name");

        if (name.empty())
        {
            continue;
        }

        mergePerson(
            persons,
            Person::withCharacterName(
                ContentClassificationSource::Tvscraper,
                PersonRole::Actor,
                name,
                normalizePersonName(name),
                getStringField(actorObject, "role")));
    }
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
    std::map<std::string, std::vector<VdrRecording*>> recordingsByAlias;

    for (VdrRecording& recording : recordings)
    {
        if (!recording.id.empty())
        {
            recordingsById[recording.id] = &recording;
        }

        const std::string aliasKey =
            recordingAliasKey(recording);

        if (!aliasKey.empty())
        {
            recordingsByAlias[aliasKey].push_back(&recording);
        }
    }

    const std::string arrayText =
        extractRecordingsArray(recordingsJson);

    for (const std::string& objectText :
         splitTopLevelObjects(arrayText))
    {
        VdrRecording* target = nullptr;
        long long number = -1;

        if (getIntegerField(objectText, "number", number) &&
            number >= 0)
        {
            const auto direct = recordingsById.find(
                std::to_string(number));

            if (direct != recordingsById.end())
            {
                target = direct->second;
            }
        }

        if (target == nullptr)
        {
            const std::string aliasKey =
                recordingAliasKey(objectText);
            const auto alias = recordingsByAlias.find(aliasKey);

            if (!aliasKey.empty() &&
                alias != recordingsByAlias.end() &&
                alias->second.size() == 1)
            {
                target = alias->second.front();
            }
        }

        if (target == nullptr)
        {
            continue;
        }

        mergeMetadata(
            target->metadata,
            RestfulApiRecordingMetadataMapper::mapRecordingObject(
                objectText));
        mergeAdditionalMediaActors(
            objectText,
            target->persons);
    }
}
