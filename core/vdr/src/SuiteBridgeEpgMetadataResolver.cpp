#include "SuiteBridgeEpgMetadataResolver.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{

class JsonValue
{
public:
    enum class Type
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    static JsonValue nullValue()
    {
        return JsonValue();
    }

    static JsonValue booleanValue(bool value)
    {
        JsonValue result;
        result.type_ = Type::Boolean;
        result.boolean_ = value;
        return result;
    }

    static JsonValue numberValue(double value)
    {
        JsonValue result;
        result.type_ = Type::Number;
        result.number_ = value;
        return result;
    }

    static JsonValue stringValue(std::string value)
    {
        JsonValue result;
        result.type_ = Type::String;
        result.string_ = std::move(value);
        return result;
    }

    static JsonValue arrayValue(std::vector<JsonValue> value)
    {
        JsonValue result;
        result.type_ = Type::Array;
        result.array_ = std::move(value);
        return result;
    }

    static JsonValue objectValue(std::map<std::string, JsonValue> value)
    {
        JsonValue result;
        result.type_ = Type::Object;
        result.object_ = std::move(value);
        return result;
    }

    Type type() const { return type_; }
    bool boolean() const { return boolean_; }
    double number() const { return number_; }
    const std::string& string() const { return string_; }
    const std::vector<JsonValue>& array() const { return array_; }
    const std::map<std::string, JsonValue>& object() const { return object_; }

    const JsonValue* member(const std::string& name) const
    {
        if (type_ != Type::Object)
        {
            return nullptr;
        }

        const auto iterator = object_.find(name);
        return iterator == object_.end() ? nullptr : &iterator->second;
    }

private:
    Type type_ = Type::Null;
    bool boolean_ = false;
    double number_ = 0.0;
    std::string string_;
    std::vector<JsonValue> array_;
    std::map<std::string, JsonValue> object_;
};

class JsonParser
{
public:
    explicit JsonParser(const std::string& text)
        : text_(text)
    {
    }

    bool parse(JsonValue& value)
    {
        if (text_.empty() || text_.size() > 8192)
        {
            return false;
        }

        skipWhitespace();
        if (!parseValue(value, 0))
        {
            return false;
        }
        skipWhitespace();
        return position_ == text_.size();
    }

private:
    static constexpr int MaximumDepth = 16;

    void skipWhitespace()
    {
        while (position_ < text_.size())
        {
            const char character = text_[position_];
            if (character != ' ' && character != '\t' &&
                character != '\r' && character != '\n')
            {
                break;
            }
            ++position_;
        }
    }

    bool parseValue(JsonValue& value, int depth)
    {
        if (depth > MaximumDepth || position_ >= text_.size())
        {
            return false;
        }

        switch (text_[position_])
        {
            case '{': return parseObject(value, depth + 1);
            case '[': return parseArray(value, depth + 1);
            case '"':
            {
                std::string parsed;
                if (!parseString(parsed))
                {
                    return false;
                }
                value = JsonValue::stringValue(std::move(parsed));
                return true;
            }
            case 't':
                if (!consumeLiteral("true")) return false;
                value = JsonValue::booleanValue(true);
                return true;
            case 'f':
                if (!consumeLiteral("false")) return false;
                value = JsonValue::booleanValue(false);
                return true;
            case 'n':
                if (!consumeLiteral("null")) return false;
                value = JsonValue::nullValue();
                return true;
            default:
                return parseNumber(value);
        }
    }

    bool parseObject(JsonValue& value, int depth)
    {
        if (position_ >= text_.size() || text_[position_] != '{')
        {
            return false;
        }
        ++position_;
        skipWhitespace();

        std::map<std::string, JsonValue> object;
        if (position_ < text_.size() && text_[position_] == '}')
        {
            ++position_;
            value = JsonValue::objectValue(std::move(object));
            return true;
        }

        while (position_ < text_.size())
        {
            std::string name;
            if (!parseString(name))
            {
                return false;
            }
            skipWhitespace();
            if (position_ >= text_.size() || text_[position_] != ':')
            {
                return false;
            }
            ++position_;
            skipWhitespace();

            JsonValue memberValue;
            if (!parseValue(memberValue, depth))
            {
                return false;
            }
            if (!object.emplace(std::move(name), std::move(memberValue)).second)
            {
                return false;
            }

            skipWhitespace();
            if (position_ >= text_.size())
            {
                return false;
            }
            if (text_[position_] == '}')
            {
                ++position_;
                value = JsonValue::objectValue(std::move(object));
                return true;
            }
            if (text_[position_] != ',')
            {
                return false;
            }
            ++position_;
            skipWhitespace();
        }

        return false;
    }

    bool parseArray(JsonValue& value, int depth)
    {
        if (position_ >= text_.size() || text_[position_] != '[')
        {
            return false;
        }
        ++position_;
        skipWhitespace();

        std::vector<JsonValue> array;
        if (position_ < text_.size() && text_[position_] == ']')
        {
            ++position_;
            value = JsonValue::arrayValue(std::move(array));
            return true;
        }

        while (position_ < text_.size())
        {
            JsonValue element;
            if (!parseValue(element, depth))
            {
                return false;
            }
            array.push_back(std::move(element));

            skipWhitespace();
            if (position_ >= text_.size())
            {
                return false;
            }
            if (text_[position_] == ']')
            {
                ++position_;
                value = JsonValue::arrayValue(std::move(array));
                return true;
            }
            if (text_[position_] != ',')
            {
                return false;
            }
            ++position_;
            skipWhitespace();
        }

        return false;
    }

    static int hexValue(char character)
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

    static bool appendUtf8(std::string& target, unsigned int value)
    {
        if (value <= 0x7f)
        {
            target.push_back(static_cast<char>(value));
            return true;
        }
        if (value <= 0x7ff)
        {
            target.push_back(static_cast<char>(0xc0 | (value >> 6)));
            target.push_back(static_cast<char>(0x80 | (value & 0x3f)));
            return true;
        }
        if (value >= 0xd800 && value <= 0xdfff)
        {
            return false;
        }
        if (value <= 0xffff)
        {
            target.push_back(static_cast<char>(0xe0 | (value >> 12)));
            target.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
            target.push_back(static_cast<char>(0x80 | (value & 0x3f)));
            return true;
        }
        return false;
    }

    bool parseString(std::string& value)
    {
        if (position_ >= text_.size() || text_[position_] != '"')
        {
            return false;
        }
        ++position_;
        value.clear();

        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);
            if (character == '"')
            {
                return true;
            }
            if (character < 0x20)
            {
                return false;
            }
            if (character != '\\')
            {
                value.push_back(static_cast<char>(character));
                continue;
            }

            if (position_ >= text_.size())
            {
                return false;
            }
            const char escaped = text_[position_++];
            switch (escaped)
            {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case 'u':
                {
                    if (position_ + 4 > text_.size())
                    {
                        return false;
                    }
                    unsigned int codepoint = 0;
                    for (int offset = 0; offset < 4; ++offset)
                    {
                        const int digit = hexValue(text_[position_ + offset]);
                        if (digit < 0)
                        {
                            return false;
                        }
                        codepoint = codepoint * 16U +
                            static_cast<unsigned int>(digit);
                    }
                    position_ += 4;
                    if (!appendUtf8(value, codepoint))
                    {
                        return false;
                    }
                    break;
                }
                default:
                    return false;
            }
        }

        return false;
    }

    bool parseNumber(JsonValue& value)
    {
        const std::size_t start = position_;
        if (position_ < text_.size() && text_[position_] == '-')
        {
            ++position_;
        }
        if (position_ >= text_.size())
        {
            return false;
        }

        if (text_[position_] == '0')
        {
            ++position_;
        }
        else
        {
            if (text_[position_] < '1' || text_[position_] > '9')
            {
                return false;
            }
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
        }

        if (position_ < text_.size() && text_[position_] == '.')
        {
            ++position_;
            const std::size_t fractionStart = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == fractionStart)
            {
                return false;
            }
        }

        if (position_ < text_.size() &&
            (text_[position_] == 'e' || text_[position_] == 'E'))
        {
            ++position_;
            if (position_ < text_.size() &&
                (text_[position_] == '+' || text_[position_] == '-'))
            {
                ++position_;
            }
            const std::size_t exponentStart = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' && text_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == exponentStart)
            {
                return false;
            }
        }

        const std::string token = text_.substr(start, position_ - start);
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (end == token.c_str() || *end != '\0' || !std::isfinite(parsed))
        {
            return false;
        }

        value = JsonValue::numberValue(parsed);
        return true;
    }

    bool consumeLiteral(const char* literal)
    {
        const std::string value(literal);
        if (text_.compare(position_, value.size(), value) != 0)
        {
            return false;
        }
        position_ += value.size();
        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
};

bool requiredBool(const JsonValue& object, const char* name, bool& value)
{
    const JsonValue* member = object.member(name);
    if (!member || member->type() != JsonValue::Type::Boolean)
    {
        return false;
    }
    value = member->boolean();
    return true;
}

bool requiredString(
    const JsonValue& object,
    const char* name,
    std::string& value)
{
    const JsonValue* member = object.member(name);
    if (!member || member->type() != JsonValue::Type::String)
    {
        return false;
    }
    value = member->string();
    return true;
}

bool requiredNumber(
    const JsonValue& object,
    const char* name,
    double& value)
{
    const JsonValue* member = object.member(name);
    if (!member || member->type() != JsonValue::Type::Number)
    {
        return false;
    }
    value = member->number();
    return true;
}

bool requiredInt(const JsonValue& object, const char* name, int& value)
{
    double number = 0.0;
    if (!requiredNumber(object, name, number) || number < 0.0 ||
        number > static_cast<double>(std::numeric_limits<int>::max()) ||
        std::floor(number) != number)
    {
        return false;
    }
    value = static_cast<int>(number);
    return true;
}

bool stringArray(
    const JsonValue& object,
    const char* name,
    std::size_t maximum,
    std::vector<std::string>& values)
{
    const JsonValue* member = object.member(name);
    if (!member || member->type() != JsonValue::Type::Array ||
        member->array().size() > maximum)
    {
        return false;
    }

    values.clear();
    for (const JsonValue& item : member->array())
    {
        if (item.type() != JsonValue::Type::String)
        {
            return false;
        }
        values.push_back(item.string());
    }
    return true;
}

EpgScraperMediaType mediaType(const std::string& value)
{
    if (value == "series") return EpgScraperMediaType::Series;
    if (value == "movie") return EpgScraperMediaType::Movie;
    return EpgScraperMediaType::None;
}

EpgScraperPersonRole personRole(const std::string& value)
{
    if (value == "actor") return EpgScraperPersonRole::Actor;
    if (value == "director") return EpgScraperPersonRole::Director;
    if (value == "writer") return EpgScraperPersonRole::Writer;
    if (value == "producer") return EpgScraperPersonRole::Producer;
    if (value == "moderator") return EpgScraperPersonRole::Moderator;
    if (value == "guest") return EpgScraperPersonRole::Guest;
    if (value == "composer") return EpgScraperPersonRole::Composer;
    if (value == "other") return EpgScraperPersonRole::Other;
    return EpgScraperPersonRole::Unknown;
}

EpgScraperImageOrientation imageOrientation(const std::string& value)
{
    if (value == "landscape") return EpgScraperImageOrientation::Landscape;
    if (value == "banner") return EpgScraperImageOrientation::Banner;
    if (value == "portrait") return EpgScraperImageOrientation::Portrait;
    return EpgScraperImageOrientation::Unknown;
}

EpgScraperArtworkOrigin artworkOrigin(const std::string& value)
{
    if (value == "primary-metadata")
    {
        return EpgScraperArtworkOrigin::PrimaryMetadata;
    }
    if (value == "external-fallback")
    {
        return EpgScraperArtworkOrigin::ExternalFallback;
    }
    return EpgScraperArtworkOrigin::Unknown;
}

EpgScraperExternalIdProvider externalIdProvider(const std::string& value)
{
    if (value == "imdb") return EpgScraperExternalIdProvider::Imdb;
    if (value == "tmdb") return EpgScraperExternalIdProvider::Tmdb;
    if (value == "tvdb") return EpgScraperExternalIdProvider::Tvdb;
    return EpgScraperExternalIdProvider::Unknown;
}

EpgScraperExternalIdScope externalIdScope(const std::string& value)
{
    if (value == "series") return EpgScraperExternalIdScope::Series;
    if (value == "season") return EpgScraperExternalIdScope::Season;
    if (value == "episode") return EpgScraperExternalIdScope::Episode;
    if (value == "movie") return EpgScraperExternalIdScope::Movie;
    return EpgScraperExternalIdScope::Unknown;
}

bool parseArtwork(const JsonValue& value, EpgScraperArtwork& artwork)
{
    if (value.type() != JsonValue::Type::Object ||
        !requiredBool(value, "available", artwork.available) ||
        !requiredString(value, "provider", artwork.provider) ||
        !requiredString(value, "path", artwork.path) ||
        !requiredInt(value, "width", artwork.width) ||
        !requiredInt(value, "height", artwork.height))
    {
        return false;
    }

    const JsonValue* origin = value.member("origin");
    if (origin)
    {
        if (origin->type() != JsonValue::Type::String)
        {
            return false;
        }
        artwork.origin = artworkOrigin(origin->string());
        if (origin->string() != "none" &&
            artwork.origin == EpgScraperArtworkOrigin::Unknown)
        {
            return false;
        }
    }
    else
    {
        // Schema-1 payloads created before artwork provenance existed remain
        // valid and are unambiguously primary TVScraper metadata.
        artwork.origin = artwork.available
            ? EpgScraperArtworkOrigin::PrimaryMetadata
            : EpgScraperArtworkOrigin::Unknown;
    }

    if (!artwork.available)
    {
        return artwork.provider == "none" &&
            artwork.origin == EpgScraperArtworkOrigin::Unknown &&
            artwork.path.empty() && artwork.width == 0 && artwork.height == 0;
    }

    // The bridge remains a TVScraper-only trust boundary. External fallback
    // artwork is introduced later behind a separate daemon-side validator.
    return artwork.origin == EpgScraperArtworkOrigin::PrimaryMetadata &&
        artwork.valid();
}

bool parseExternalIds(
    const JsonValue& root,
    std::vector<EpgScraperExternalId>& externalIds)
{
    const JsonValue* member = root.member("externalIds");
    externalIds.clear();
    if (!member)
    {
        // Optional schema-1 extension for rolling plugin/daemon upgrades.
        return true;
    }
    if (member->type() != JsonValue::Type::Array ||
        member->array().size() > 8)
    {
        return false;
    }

    for (const JsonValue& item : member->array())
    {
        if (item.type() != JsonValue::Type::Object)
        {
            return false;
        }

        std::string provider;
        std::string scope;
        EpgScraperExternalId externalId;
        if (!requiredString(item, "provider", provider) ||
            !requiredString(item, "scope", scope) ||
            !requiredString(item, "value", externalId.value) ||
            externalId.value.size() > 128)
        {
            return false;
        }

        externalId.provider = externalIdProvider(provider);
        externalId.scope = externalIdScope(scope);
        if (!externalId.valid())
        {
            return false;
        }

        for (const EpgScraperExternalId& existing : externalIds)
        {
            if (existing.provider == externalId.provider &&
                existing.scope == externalId.scope &&
                existing.value == externalId.value)
            {
                return false;
            }
        }
        externalIds.push_back(std::move(externalId));
    }

    return true;
}

bool parsePeople(const JsonValue& root, std::vector<EpgScraperPerson>& people)
{
    const JsonValue* member = root.member("people");
    if (!member || member->type() != JsonValue::Type::Array ||
        member->array().size() > 12)
    {
        return false;
    }

    people.clear();
    for (const JsonValue& item : member->array())
    {
        if (item.type() != JsonValue::Type::Object)
        {
            return false;
        }

        EpgScraperPerson person;
        std::string role;
        const JsonValue* image = item.member("image");
        if (!requiredString(item, "role", role) ||
            !requiredString(item, "name", person.name) ||
            !requiredString(item, "characterName", person.characterName) ||
            !image || !parseArtwork(*image, person.image))
        {
            return false;
        }
        person.role = personRole(role);
        if (!person.valid())
        {
            return false;
        }
        people.push_back(std::move(person));
    }
    return true;
}

bool parseImages(const JsonValue& root, std::vector<EpgScraperImage>& images)
{
    const JsonValue* member = root.member("images");
    if (!member || member->type() != JsonValue::Type::Array ||
        member->array().size() > 8)
    {
        return false;
    }

    images.clear();
    for (const JsonValue& item : member->array())
    {
        if (item.type() != JsonValue::Type::Object)
        {
            return false;
        }

        EpgScraperImage image;
        std::string orientation;
        const JsonValue* artwork = item.member("artwork");
        if (!requiredString(item, "orientation", orientation) ||
            !artwork || !parseArtwork(*artwork, image.artwork))
        {
            return false;
        }
        image.orientation = imageOrientation(orientation);
        if (!image.valid())
        {
            return false;
        }
        images.push_back(std::move(image));
    }
    return true;
}

bool parseMetadata(
    const std::string& payload,
    const std::string& backendId,
    const VdrEvent& event,
    bool& found,
    EpgScraperMetadata& metadata)
{
    JsonValue root;
    JsonParser parser(payload);
    if (!parser.parse(root) || root.type() != JsonValue::Type::Object)
    {
        return false;
    }

    int schema = 0;
    std::string provider;
    if (!requiredInt(root, "schema", schema) || schema != 1 ||
        !requiredBool(root, "found", found) ||
        !requiredString(root, "provider", provider))
    {
        return false;
    }

    if (!found)
    {
        return provider == "none";
    }
    if (provider != "tvscraper")
    {
        return false;
    }

    metadata.backendId = backendId;
    metadata.channelId = event.channelId;
    metadata.eventId = event.id;
    metadata.provider = provider;

    std::string mediaTypeValue;
    double popularity = 0.0;
    double voteAverage = 0.0;
    const JsonValue* preferredArtwork = root.member("preferredArtwork");

    if (!requiredString(root, "mediaType", mediaTypeValue) ||
        !requiredInt(root, "providerId", metadata.providerId) ||
        !requiredInt(root, "seasonNumber", metadata.seasonNumber) ||
        !requiredInt(root, "episodeNumber", metadata.episodeNumber) ||
        !requiredInt(root, "absoluteEpisodeNumber", metadata.absoluteEpisodeNumber) ||
        !requiredInt(root, "runtimeMinutes", metadata.runtimeMinutes) ||
        !requiredInt(root, "durationDeviationMinutes", metadata.durationDeviationMinutes) ||
        !requiredInt(root, "scraperHd", metadata.scraperHd) ||
        !requiredInt(root, "scraperLanguage", metadata.scraperLanguage) ||
        !requiredNumber(root, "popularity", popularity) || popularity < 0.0 ||
        !requiredNumber(root, "voteAverage", voteAverage) || voteAverage < 0.0 ||
        !requiredInt(root, "voteCount", metadata.voteCount) ||
        !requiredBool(root, "adult", metadata.adult) ||
        !requiredInt(root, "collectionId", metadata.collectionId) ||
        !requiredInt(root, "lastSeason", metadata.lastSeason) ||
        !requiredString(root, "title", metadata.title) ||
        !requiredString(root, "originalTitle", metadata.originalTitle) ||
        !requiredString(root, "episodeName", metadata.episodeName) ||
        !requiredString(root, "tagline", metadata.tagline) ||
        !requiredString(root, "overview", metadata.overview) ||
        !requiredString(root, "releaseDate", metadata.releaseDate) ||
        !requiredString(root, "firstAired", metadata.firstAired) ||
        !requiredString(root, "imdbId", metadata.imdbId) ||
        !parseExternalIds(root, metadata.externalIds) ||
        !requiredString(root, "status", metadata.status) ||
        !requiredString(root, "collectionName", metadata.collectionName) ||
        !stringArray(root, "genres", 12, metadata.genres) ||
        !stringArray(root, "productionCountries", 8, metadata.productionCountries) ||
        !stringArray(root, "networks", 8, metadata.networks) ||
        !preferredArtwork ||
        !parseArtwork(*preferredArtwork, metadata.preferredArtwork) ||
        !parsePeople(root, metadata.people) ||
        !parseImages(root, metadata.images))
    {
        return false;
    }

    metadata.mediaType = mediaType(mediaTypeValue);
    metadata.popularity = popularity;
    metadata.voteAverage = voteAverage;
    return metadata.valid();
}

}

SuiteBridgeEpgMetadataResolver::SuiteBridgeEpgMetadataResolver(
    ISuiteBridgeMetadataTransport& transport)
    : transport_(transport)
{
}

EpgScraperMetadataResolution SuiteBridgeEpgMetadataResolver::resolve(
    const std::string& backendId,
    const VdrEvent& event)
{
    EpgScraperMetadataResolution resolution;
    if (backendId.empty() || event.channelId.empty() || event.id.empty())
    {
        return resolution;
    }

    const SuiteBridgeMetadataCommandReply reply =
        transport_.requestMetadata(event.channelId, event.id);
    if (!reply.transportSucceeded)
    {
        return resolution;
    }

    resolution.attempted = true;
    if (reply.replyCode != 250)
    {
        return resolution;
    }

    bool found = false;
    EpgScraperMetadata metadata;
    if (!parseMetadata(reply.payload, backendId, event, found, metadata))
    {
        resolution.attempted = false;
        return resolution;
    }

    resolution.found = found;
    if (found)
    {
        resolution.metadata = std::move(metadata);
    }
    return resolution;
}
