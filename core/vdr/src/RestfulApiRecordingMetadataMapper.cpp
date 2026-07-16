#include "RestfulApiRecordingMetadataMapper.h"

#include "JsonStringDecoder.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <initializer_list>
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

    std::size_t quoteStart = skipWhitespace(objectText, colon + 1);

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

long long getIntegerField(
    const std::string& objectText,
    const std::string& fieldName,
    const long long fallback = 0)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return fallback;
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());

    if (colon == std::string::npos)
    {
        return fallback;
    }

    std::size_t position = skipWhitespace(objectText, colon + 1);

    if (position >= objectText.size())
    {
        return fallback;
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
        return fallback;
    }

    long long value = 0;

    while (position < objectText.size() &&
           std::isdigit(static_cast<unsigned char>(objectText[position])))
    {
        value = value * 10 + (objectText[position] - '0');
        ++position;
    }

    return negative ? -value : value;
}

double getDoubleField(
    const std::string& objectText,
    const std::string& fieldName,
    const double fallback = 0.0)
{
    const std::string key = "\"" + fieldName + "\"";
    const std::size_t keyPosition = objectText.find(key);

    if (keyPosition == std::string::npos)
    {
        return fallback;
    }

    const std::size_t colon =
        objectText.find(':', keyPosition + key.size());

    if (colon == std::string::npos)
    {
        return fallback;
    }

    const std::size_t position = skipWhitespace(objectText, colon + 1);

    if (position >= objectText.size())
    {
        return fallback;
    }

    const char* begin = objectText.c_str() + position;
    char* end = nullptr;
    const double value = std::strtod(begin, &end);

    if (end == begin)
    {
        return fallback;
    }

    return value;
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

    if (colon == std::string::npos)
    {
        return {};
    }

    const std::size_t objectStart = objectText.find('{', colon + 1);

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

    if (colon == std::string::npos)
    {
        return {};
    }

    const std::size_t arrayStart = objectText.find('[', colon + 1);

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

std::string firstStringField(
    const std::string& objectText,
    const std::initializer_list<const char*> fieldNames)
{
    for (const char* fieldName : fieldNames)
    {
        const std::string value =
            getStringField(objectText, fieldName);

        if (!value.empty())
        {
            return value;
        }
    }

    return {};
}

long long firstIntegerField(
    const std::string& objectText,
    const std::initializer_list<const char*> fieldNames)
{
    for (const char* fieldName : fieldNames)
    {
        const long long value =
            getIntegerField(objectText, fieldName, 0);

        if (value != 0)
        {
            return value;
        }
    }

    return 0;
}

double firstPositiveDoubleField(
    const std::string& objectText,
    const std::initializer_list<const char*> fieldNames)
{
    for (const char* fieldName : fieldNames)
    {
        const double value =
            getDoubleField(objectText, fieldName, 0.0);

        if (value > 0.0)
        {
            return value;
        }
    }

    return 0.0;
}

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    return value;
}

std::string trim(std::string value)
{
    const auto isNotSpace = [](const unsigned char character)
    {
        return !std::isspace(character);
    };

    value.erase(
        value.begin(),
        std::find_if(value.begin(), value.end(), isNotSpace));
    value.erase(
        std::find_if(value.rbegin(), value.rend(), isNotSpace).base(),
        value.end());

    return value;
}

std::string normalizeProviderReference(std::string reference)
{
    reference = trim(reference);

    if (reference.empty())
    {
        return {};
    }

    std::replace(reference.begin(), reference.end(), '\\', '/');

    while (reference.compare(0, 2, "./") == 0)
    {
        reference.erase(0, 2);
    }

    const std::string lower = lowerAscii(reference);

    if (reference.empty() ||
        reference.front() == '/' ||
        lower.find("://") != std::string::npos ||
        lower.find("%2e") != std::string::npos ||
        lower.find("%2f") != std::string::npos ||
        lower.find("%5c") != std::string::npos)
    {
        return {};
    }

    std::string normalized;
    std::string segment;

    for (std::size_t index = 0;
         index <= reference.size();
         ++index)
    {
        const char character =
            index < reference.size() ? reference[index] : '/';

        if (character == '/')
        {
            if (segment == "..")
            {
                return {};
            }

            if (!segment.empty() && segment != ".")
            {
                if (!normalized.empty())
                {
                    normalized.push_back('/');
                }

                normalized += segment;
            }

            segment.clear();
            continue;
        }

        segment.push_back(character);
    }

    return normalized;
}

std::string positiveId(const long long value)
{
    return value > 0 ? std::to_string(value) : std::string();
}

void addArtwork(
    VdrRecordingMetadata& metadata,
    const VdrRecordingArtworkKind kind,
    const std::string& rawReference,
    const int width = 0,
    const int height = 0)
{
    const std::string reference =
        normalizeProviderReference(rawReference);

    if (reference.empty())
    {
        return;
    }

    for (const VdrRecordingArtworkRef& existing : metadata.artwork)
    {
        if (existing.kind == kind &&
            existing.reference == reference)
        {
            return;
        }
    }

    VdrRecordingArtworkRef artwork;
    artwork.kind = kind;
    artwork.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    artwork.reference = reference;
    artwork.width = width > 0 ? width : 0;
    artwork.height = height > 0 ? height : 0;
    artwork.temporary = true;
    metadata.artwork.push_back(artwork);
}

void addArtworkArray(
    VdrRecordingMetadata& metadata,
    const std::string& additionalMedia,
    const char* fieldName,
    const VdrRecordingArtworkKind kind)
{
    const std::string arrayText =
        getArrayField(additionalMedia, fieldName);

    for (const std::string& imageObject :
         splitTopLevelObjects(arrayText))
    {
        addArtwork(
            metadata,
            kind,
            getStringField(imageObject, "path"),
            static_cast<int>(
                getIntegerField(imageObject, "width", 0)),
            static_cast<int>(
                getIntegerField(imageObject, "height", 0)));
    }
}

void mapMovieMetadata(
    const std::string& additionalMedia,
    VdrRecordingMetadata& metadata)
{
    VdrRecordingProviderMetadata& provider = metadata.provider;
    provider.contentKind = VdrRecordingContentKind::Movie;
    provider.movieId = positiveId(
        firstIntegerField(
            additionalMedia,
            {"movie_id", "movieId"}));
    provider.title = firstStringField(
        additionalMedia,
        {"title", "movie_title"});
    provider.originalTitle = firstStringField(
        additionalMedia,
        {"original_title", "movie_original_title"});
    provider.tagline = firstStringField(
        additionalMedia,
        {"tagline", "movie_tagline"});
    provider.overview = firstStringField(
        additionalMedia,
        {"overview", "movie_overview"});
    provider.genreText = firstStringField(
        additionalMedia,
        {"genres", "movie_genres"});
    provider.releaseDate = firstStringField(
        additionalMedia,
        {"release_date", "movie_release_date"});
    provider.runtimeMinutes = static_cast<int>(
        firstIntegerField(
            additionalMedia,
            {"runtime", "movie_runtime"}));
    provider.rating = firstPositiveDoubleField(
        additionalMedia,
        {"vote_average", "movie_vote_average", "rating"});

    addArtwork(
        metadata,
        VdrRecordingArtworkKind::Poster,
        firstStringField(
            additionalMedia,
            {"poster", "movie_poster"}));
    addArtwork(
        metadata,
        VdrRecordingArtworkKind::Fanart,
        firstStringField(
            additionalMedia,
            {"fanart", "movie_fanart"}));
}

void mapSeriesMetadata(
    const std::string& additionalMedia,
    VdrRecordingMetadata& metadata)
{
    VdrRecordingProviderMetadata& provider = metadata.provider;
    provider.contentKind = VdrRecordingContentKind::SeriesEpisode;
    provider.seriesId = positiveId(
        firstIntegerField(
            additionalMedia,
            {"series_id", "seriesId"}));
    provider.episodeId = positiveId(
        firstIntegerField(
            additionalMedia,
            {"episode_id", "episodeId"}));
    provider.seriesTitle = firstStringField(
        additionalMedia,
        {"name", "series_name"});
    provider.episodeTitle = firstStringField(
        additionalMedia,
        {"episode_name", "episode_title"});
    provider.title = !provider.episodeTitle.empty()
        ? provider.episodeTitle
        : provider.seriesTitle;
    provider.overview = firstStringField(
        additionalMedia,
        {"episode_overview", "overview", "series_overview"});
    provider.genreText = firstStringField(
        additionalMedia,
        {"genre", "series_genre"});
    provider.releaseDate = firstStringField(
        additionalMedia,
        {"episode_first_aired", "first_aired"});
    provider.seasonNumber = static_cast<int>(
        firstIntegerField(
            additionalMedia,
            {"episode_season", "season_number"}));
    provider.episodeNumber = static_cast<int>(
        firstIntegerField(
            additionalMedia,
            {"episode_number", "number"}));
    provider.rating = firstPositiveDoubleField(
        additionalMedia,
        {"episode_rating", "rating"});

    addArtwork(
        metadata,
        VdrRecordingArtworkKind::Still,
        getStringField(additionalMedia, "episode_image"));
    addArtworkArray(
        metadata,
        additionalMedia,
        "posters",
        VdrRecordingArtworkKind::Poster);
    addArtworkArray(
        metadata,
        additionalMedia,
        "banners",
        VdrRecordingArtworkKind::Banner);
    addArtworkArray(
        metadata,
        additionalMedia,
        "fanarts",
        VdrRecordingArtworkKind::Fanart);
}

bool hasProviderEvidence(
    const VdrRecordingMetadata& metadata)
{
    const VdrRecordingProviderMetadata& provider = metadata.provider;

    return provider.contentKind != VdrRecordingContentKind::Unknown ||
           !provider.movieId.empty() ||
           !provider.seriesId.empty() ||
           !provider.episodeId.empty() ||
           !provider.title.empty() ||
           !provider.overview.empty() ||
           metadata.hasArtwork();
}

}

VdrRecordingMetadata
RestfulApiRecordingMetadataMapper::mapRecordingObject(
    const std::string& recordingObjectJson)
{
    VdrRecordingMetadata metadata;

    metadata.native.eventTitle =
        getStringField(recordingObjectJson, "event_title");
    metadata.native.shortText =
        getStringField(recordingObjectJson, "event_short_text");
    metadata.native.description =
        getStringField(recordingObjectJson, "event_description");

    const std::string additionalMedia =
        getObjectField(recordingObjectJson, "additional_media");

    if (additionalMedia.empty())
    {
        return metadata;
    }

    std::string type = lowerAscii(
        firstStringField(
            additionalMedia,
            {"type", "scraper"}));

    if (type.empty())
    {
        if (firstIntegerField(
                additionalMedia,
                {"movie_id", "movieId"}) > 0)
        {
            type = "movie";
        }
        else if (firstIntegerField(
                     additionalMedia,
                     {"series_id", "seriesId"}) > 0)
        {
            type = "series";
        }
    }

    if (type == "movie")
    {
        mapMovieMetadata(additionalMedia, metadata);
    }
    else if (type == "series" ||
             type == "episode" ||
             type == "series-episode")
    {
        mapSeriesMetadata(additionalMedia, metadata);
    }

    if (hasProviderEvidence(metadata))
    {
        metadata.provider.source =
            VdrRecordingMetadataSource::RestfulApiScraperBridge;
    }

    return metadata;
}
