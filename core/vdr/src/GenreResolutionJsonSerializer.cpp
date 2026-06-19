#include "GenreResolutionJsonSerializer.h"

#include "GenreLocalization.h"

#include <sstream>

namespace
{
std::string sourceToString(
    ContentClassificationSource source)
{
    switch (source)
    {
    case ContentClassificationSource::Epg:
        return "epg";
    case ContentClassificationSource::Dvb:
        return "dvb";
    case ContentClassificationSource::Tvscraper:
        return "tvscraper";
    case ContentClassificationSource::Scraper2Vdr:
        return "scraper2vdr";
    case ContentClassificationSource::Tmdb:
        return "tmdb";
    case ContentClassificationSource::Tvdb:
        return "tvdb";
    case ContentClassificationSource::Imdb:
        return "imdb";
    case ContentClassificationSource::User:
        return "user";
    case ContentClassificationSource::Folder:
        return "folder";
    case ContentClassificationSource::Derived:
        return "derived";
    }

    return "unknown";
}

void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"':
            json << "\\\"";
            break;
        case '\\':
            json << "\\\\";
            break;
        case '\n':
            json << "\\n";
            break;
        case '\r':
            json << "\\r";
            break;
        case '\t':
            json << "\\t";
            break;
        default:
            json << character;
            break;
        }
    }

    json << '"';
}

void appendGenre(
    std::ostringstream& json,
    const GenreClassification& genre,
    const std::string& locale,
    bool localized)
{
    GenreLocalization localization;

    json << '{';

    appendQuoted(json, "canonicalId");
    json << ':';
    appendQuoted(json, genre.normalizedValue());

    if (localized)
    {
        json << ',';
        appendQuoted(json, "label");
        json << ':';
        appendQuoted(
            json,
            localization.label(
                genre.normalizedValue(),
                locale));

        json << ',';
        appendQuoted(json, "locale");
        json << ':';
        appendQuoted(json, locale);
    }

    json << ',';
    appendQuoted(json, "source");
    json << ':';
    appendQuoted(json, sourceToString(genre.source()));

    json << ',';
    appendQuoted(json, "originalValue");
    json << ':';
    appendQuoted(json, genre.originalValue());

    json << ',';
    appendQuoted(json, "normalizedValue");
    json << ':';
    appendQuoted(json, genre.normalizedValue());

    json << ',';
    appendQuoted(json, "confidence");
    json << ':';
    json << genre.confidence();

    json << ',';
    appendQuoted(json, "providerReference");
    json << ':';
    appendQuoted(json, genre.providerReference());

    json << '}';
}

std::string serializeInternal(
    const GenreResolutionResult& result,
    const std::string& locale,
    bool localized)
{
    std::ostringstream json;

    json << '{';

    appendQuoted(json, "resolved");
    json << ':';
    json << (result.isResolved() ? "true" : "false");

    json << ',';
    appendQuoted(json, "primaryGenre");
    json << ':';

    if (result.isResolved())
    {
        appendGenre(
            json,
            result.primaryGenre(),
            locale,
            localized);
    }
    else
    {
        json << "null";
    }

    json << ',';
    appendQuoted(json, "evidence");
    json << ':';
    json << '[';

    const std::vector<GenreClassification>& evidence = result.evidence();

    for (std::size_t index = 0; index < evidence.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendGenre(
            json,
            evidence[index],
            locale,
            localized);
    }

    json << ']';

    json << '}';

    return json.str();
}
}

std::string GenreResolutionJsonSerializer::serialize(
    const GenreResolutionResult& result) const
{
    return serializeInternal(
        result,
        "",
        false);
}

std::string GenreResolutionJsonSerializer::serializeLocalized(
    const GenreResolutionResult& result,
    const std::string& locale) const
{
    return serializeInternal(
        result,
        locale,
        true);
}
