#include "EpgMetadataJsonParser.h"

#include <cctype>
#include <cstdlib>
#include <limits>

namespace
{
constexpr std::size_t kMaximumPayloadBytes = 32768;
constexpr std::size_t kMaximumTitleBytes = 256;
constexpr std::size_t kMaximumShortTextBytes = 512;
constexpr std::size_t kMaximumOverviewBytes = 2048;
constexpr std::size_t kMaximumPathBytes = 4096;
constexpr std::size_t kMaximumListValueBytes = 128;
constexpr std::size_t kMaximumPersonNameBytes = 160;
constexpr std::size_t kMaximumListValues = 12;
constexpr std::size_t kMaximumPersons = 12;
constexpr std::size_t kMaximumImages = 6;
constexpr int kMaximumImageDimension = 100000;

int hexDigitValue(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }
    if (character >= 'a' && character <= 'f')
    {
        return character - 'a' + 10;
    }
    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }
    return -1;
}

EpgMetadataMediaType mediaTypeFromString(const std::string& value)
{
    if (value == "series")
    {
        return EpgMetadataMediaType::Series;
    }
    if (value == "movie")
    {
        return EpgMetadataMediaType::Movie;
    }
    return EpgMetadataMediaType::None;
}

EpgMetadataImageOrientation orientationFromString(const std::string& value)
{
    if (value == "landscape")
    {
        return EpgMetadataImageOrientation::Landscape;
    }
    if (value == "banner")
    {
        return EpgMetadataImageOrientation::Banner;
    }
    if (value == "portrait")
    {
        return EpgMetadataImageOrientation::Portrait;
    }
    return EpgMetadataImageOrientation::None;
}

bool validRole(const std::string& value)
{
    return value == "actor" ||
        value == "director" ||
        value == "writer" ||
        value == "producer" ||
        value == "moderator" ||
        value == "guest" ||
        value == "composer" ||
        value == "other" ||
        value == "unknown";
}

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& input)
        : input_(input)
    {
    }

    bool beginObject()
    {
        return consume('{');
    }

    bool endObject()
    {
        return consume('}');
    }

    bool beginArray()
    {
        return consume('[');
    }

    bool endArray()
    {
        return consume(']');
    }

    bool comma()
    {
        return consume(',');
    }

    bool key(const char* expected)
    {
        std::string actual;
        return parseString(actual, 80) &&
            actual == expected &&
            consume(':');
    }

    bool parseString(std::string& result, std::size_t maximumBytes)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != '"')
        {
            return false;
        }
        ++position_;

        result.clear();
        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);

            if (character == '"')
            {
                return result.size() <= maximumBytes;
            }

            if (character < 0x20)
            {
                return false;
            }

            if (character != '\\')
            {
                result.push_back(static_cast<char>(character));
                if (result.size() > maximumBytes)
                {
                    return false;
                }
                continue;
            }

            if (position_ >= input_.size())
            {
                return false;
            }

            const char escaped = input_[position_++];
            switch (escaped)
            {
            case '"': result.push_back('"'); break;
            case '\\': result.push_back('\\'); break;
            case '/': result.push_back('/'); break;
            case 'b': result.push_back('\b'); break;
            case 'f': result.push_back('\f'); break;
            case 'n': result.push_back('\n'); break;
            case 'r': result.push_back('\r'); break;
            case 't': result.push_back('\t'); break;
            case 'u':
            {
                if (position_ + 4 > input_.size())
                {
                    return false;
                }

                int value = 0;
                for (int index = 0; index < 4; ++index)
                {
                    const int digit = hexDigitValue(input_[position_ + index]);
                    if (digit < 0)
                    {
                        return false;
                    }
                    value = value * 16 + digit;
                }
                position_ += 4;

                if (value > 0x7f)
                {
                    return false;
                }
                result.push_back(static_cast<char>(value));
                break;
            }
            default:
                return false;
            }

            if (result.size() > maximumBytes)
            {
                return false;
            }
        }

        return false;
    }

    bool parseBool(bool& value)
    {
        skipWhitespace();
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            value = true;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            value = false;
            return true;
        }
        return false;
    }

    bool parseInt(int& value)
    {
        skipWhitespace();
        if (position_ >= input_.size())
        {
            return false;
        }

        const char* begin = input_.c_str() + position_;
        char* end = nullptr;
        const long parsed = std::strtol(begin, &end, 10);
        if (end == begin ||
            parsed < 0 ||
            parsed > std::numeric_limits<int>::max())
        {
            return false;
        }

        position_ += static_cast<std::size_t>(end - begin);
        value = static_cast<int>(parsed);
        return true;
    }

    bool parseDouble(double& value)
    {
        skipWhitespace();
        if (position_ >= input_.size())
        {
            return false;
        }

        const char* begin = input_.c_str() + position_;
        char* end = nullptr;
        const double parsed = std::strtod(begin, &end);
        if (end == begin || parsed < 0.0 || parsed > 10.0)
        {
            return false;
        }

        position_ += static_cast<std::size_t>(end - begin);
        value = parsed;
        return true;
    }

    bool parseNull()
    {
        skipWhitespace();
        if (input_.compare(position_, 4, "null") != 0)
        {
            return false;
        }
        position_ += 4;
        return true;
    }

    bool nextIs(char character)
    {
        skipWhitespace();
        return position_ < input_.size() && input_[position_] == character;
    }

    bool finished()
    {
        skipWhitespace();
        return position_ == input_.size();
    }

private:
    bool consume(char expected)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != expected)
        {
            return false;
        }
        ++position_;
        return true;
    }

    void skipWhitespace()
    {
        while (position_ < input_.size() &&
            std::isspace(static_cast<unsigned char>(input_[position_])))
        {
            ++position_;
        }
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

bool parseStringArray(
    JsonCursor& cursor,
    std::vector<std::string>& values)
{
    if (!cursor.beginArray())
    {
        return false;
    }

    values.clear();
    if (cursor.nextIs(']'))
    {
        return cursor.endArray();
    }

    while (values.size() < kMaximumListValues)
    {
        std::string value;
        if (!cursor.parseString(value, kMaximumListValueBytes) || value.empty())
        {
            return false;
        }
        values.push_back(std::move(value));

        if (cursor.nextIs(']'))
        {
            return cursor.endArray();
        }
        if (!cursor.comma())
        {
            return false;
        }
    }

    return false;
}

bool parseImage(JsonCursor& cursor, EpgMetadataImage& image)
{
    if (!cursor.beginObject() || !cursor.key("orientation"))
    {
        return false;
    }

    std::string orientation;
    if (!cursor.parseString(orientation, 32) ||
        !cursor.comma() ||
        !cursor.key("path") ||
        !cursor.parseString(image.path, kMaximumPathBytes) ||
        !cursor.comma() ||
        !cursor.key("width") ||
        !cursor.parseInt(image.width) ||
        !cursor.comma() ||
        !cursor.key("height") ||
        !cursor.parseInt(image.height) ||
        !cursor.endObject())
    {
        return false;
    }

    image.orientation = orientationFromString(orientation);
    return image.valid() &&
        image.width <= kMaximumImageDimension &&
        image.height <= kMaximumImageDimension;
}

bool parseOptionalImage(JsonCursor& cursor, EpgMetadataImage& image)
{
    if (cursor.nextIs('n'))
    {
        return cursor.parseNull();
    }
    return parseImage(cursor, image);
}

bool parsePersons(
    JsonCursor& cursor,
    std::vector<EpgMetadataPerson>& persons)
{
    if (!cursor.beginArray())
    {
        return false;
    }

    persons.clear();
    if (cursor.nextIs(']'))
    {
        return cursor.endArray();
    }

    while (persons.size() < kMaximumPersons)
    {
        EpgMetadataPerson person;
        if (!cursor.beginObject() ||
            !cursor.key("role") ||
            !cursor.parseString(person.role, 32) ||
            !validRole(person.role) ||
            !cursor.comma() ||
            !cursor.key("name") ||
            !cursor.parseString(person.name, kMaximumPersonNameBytes) ||
            person.name.empty() ||
            !cursor.comma() ||
            !cursor.key("characterName") ||
            !cursor.parseString(
                person.characterName,
                kMaximumPersonNameBytes) ||
            !cursor.comma() ||
            !cursor.key("image") ||
            !parseOptionalImage(cursor, person.image) ||
            !cursor.endObject())
        {
            return false;
        }

        persons.push_back(std::move(person));
        if (cursor.nextIs(']'))
        {
            return cursor.endArray();
        }
        if (!cursor.comma())
        {
            return false;
        }
    }

    return false;
}

bool parseImages(
    JsonCursor& cursor,
    std::vector<EpgMetadataImage>& images)
{
    if (!cursor.beginArray())
    {
        return false;
    }

    images.clear();
    if (cursor.nextIs(']'))
    {
        return cursor.endArray();
    }

    while (images.size() < kMaximumImages)
    {
        EpgMetadataImage image;
        if (!parseImage(cursor, image))
        {
            return false;
        }
        images.push_back(std::move(image));

        if (cursor.nextIs(']'))
        {
            return cursor.endArray();
        }
        if (!cursor.comma())
        {
            return false;
        }
    }

    return false;
}

bool parseRequiredStringField(
    JsonCursor& cursor,
    const char* name,
    std::string& value,
    std::size_t maximumBytes)
{
    return cursor.comma() &&
        cursor.key(name) &&
        cursor.parseString(value, maximumBytes);
}

bool parseRequiredIntField(
    JsonCursor& cursor,
    const char* name,
    int& value)
{
    return cursor.comma() && cursor.key(name) && cursor.parseInt(value);
}
}

EpgMetadataRecord EpgMetadataJsonParser::parse(
    const std::string& payload,
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    long long resolvedAt) const
{
    EpgMetadataRecord metadata;
    if (payload.empty() ||
        payload.size() > kMaximumPayloadBytes ||
        backendId.empty() ||
        channelId.empty() ||
        eventId.empty() ||
        resolvedAt <= 0)
    {
        return metadata;
    }

    JsonCursor cursor(payload);
    int schema = 0;
    bool found = false;

    if (!cursor.beginObject() ||
        !cursor.key("schema") ||
        !cursor.parseInt(schema) ||
        schema != 1 ||
        !cursor.comma() ||
        !cursor.key("found") ||
        !cursor.parseBool(found) ||
        !cursor.comma() ||
        !cursor.key("provider") ||
        !cursor.parseString(metadata.provider, 32))
    {
        return {};
    }

    if (!found)
    {
        if (metadata.provider != "none" ||
            !cursor.endObject() ||
            !cursor.finished())
        {
            return {};
        }
        return {};
    }

    std::string mediaType;
    if (metadata.provider != "tvscraper" ||
        !parseRequiredStringField(
            cursor,
            "mediaType",
            mediaType,
            16) ||
        !parseRequiredIntField(
            cursor,
            "databaseId",
            metadata.providerDatabaseId) ||
        !parseRequiredStringField(cursor, "title", metadata.title, kMaximumTitleBytes) ||
        !parseRequiredStringField(cursor, "originalTitle", metadata.originalTitle, kMaximumTitleBytes) ||
        !parseRequiredStringField(cursor, "episodeTitle", metadata.episodeTitle, kMaximumTitleBytes) ||
        !parseRequiredStringField(cursor, "tagline", metadata.tagline, kMaximumShortTextBytes) ||
        !parseRequiredStringField(cursor, "overview", metadata.overview, kMaximumOverviewBytes) ||
        !parseRequiredStringField(cursor, "episodeOverview", metadata.episodeOverview, kMaximumOverviewBytes) ||
        !parseRequiredStringField(cursor, "releaseDate", metadata.releaseDate, kMaximumShortTextBytes) ||
        !parseRequiredStringField(cursor, "firstAired", metadata.firstAired, kMaximumShortTextBytes) ||
        !parseRequiredStringField(cursor, "imdbId", metadata.imdbId, kMaximumShortTextBytes) ||
        !parseRequiredIntField(cursor, "collectionId", metadata.collectionId) ||
        !parseRequiredStringField(cursor, "collectionName", metadata.collectionName, kMaximumTitleBytes) ||
        !parseRequiredStringField(cursor, "status", metadata.status, kMaximumShortTextBytes) ||
        !parseRequiredIntField(cursor, "runtimeMinutes", metadata.runtimeMinutes) ||
        !parseRequiredIntField(cursor, "seasonNumber", metadata.seasonNumber) ||
        !parseRequiredIntField(cursor, "episodeNumber", metadata.episodeNumber) ||
        !parseRequiredIntField(cursor, "absoluteEpisodeNumber", metadata.absoluteEpisodeNumber) ||
        !parseRequiredIntField(cursor, "lastSeason", metadata.lastSeason) ||
        !cursor.comma() ||
        !cursor.key("adult") ||
        !cursor.parseBool(metadata.adult) ||
        !cursor.comma() ||
        !cursor.key("voteAverage") ||
        !cursor.parseDouble(metadata.voteAverage) ||
        !parseRequiredIntField(cursor, "voteCount", metadata.voteCount) ||
        !cursor.comma() ||
        !cursor.key("genres") ||
        !parseStringArray(cursor, metadata.genres) ||
        !cursor.comma() ||
        !cursor.key("productionCountries") ||
        !parseStringArray(cursor, metadata.productionCountries) ||
        !cursor.comma() ||
        !cursor.key("networks") ||
        !parseStringArray(cursor, metadata.networks) ||
        !cursor.comma() ||
        !cursor.key("persons") ||
        !parsePersons(cursor, metadata.persons) ||
        !cursor.comma() ||
        !cursor.key("images") ||
        !parseImages(cursor, metadata.images) ||
        !cursor.endObject() ||
        !cursor.finished())
    {
        return {};
    }

    metadata.mediaType = mediaTypeFromString(mediaType);
    metadata.backendId = backendId;
    metadata.channelId = channelId;
    metadata.eventId = eventId;
    metadata.sourcePayload = payload;
    metadata.resolvedAt = resolvedAt;

    if (!metadata.valid())
    {
        return {};
    }

    return metadata;
}
