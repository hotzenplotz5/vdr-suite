#include "ContentRatingResolutionJsonSerializer.h"

#include <sstream>

namespace
{
std::string sourceToString(ContentClassificationSource source)
{
    switch (source)
    {
    case ContentClassificationSource::Epg: return "epg";
    case ContentClassificationSource::Dvb: return "dvb";
    case ContentClassificationSource::Tvscraper: return "tvscraper";
    case ContentClassificationSource::Scraper2Vdr: return "scraper2vdr";
    case ContentClassificationSource::Tmdb: return "tmdb";
    case ContentClassificationSource::Tvdb: return "tvdb";
    case ContentClassificationSource::Imdb: return "imdb";
    case ContentClassificationSource::User: return "user";
    case ContentClassificationSource::Folder: return "folder";
    case ContentClassificationSource::Derived: return "derived";
    }

    return "unknown";
}

std::string systemToString(ContentRatingSystem system)
{
    switch (system)
    {
    case ContentRatingSystem::Unknown: return "unknown";
    case ContentRatingSystem::Fsk: return "fsk";
    case ContentRatingSystem::Usk: return "usk";
    case ContentRatingSystem::TvParentalGuideline: return "tv-parental-guideline";
    case ContentRatingSystem::ProviderSpecific: return "provider-specific";
    case ContentRatingSystem::UserDefined: return "user-defined";
    }

    return "unknown";
}

void appendQuoted(std::ostringstream& json, const std::string& value)
{
    json << '"';

    for (const char character : value)
    {
        switch (character)
        {
        case '"': json << "\\\""; break;
        case '\\': json << "\\\\"; break;
        case '\n': json << "\\n"; break;
        case '\r': json << "\\r"; break;
        case '\t': json << "\\t"; break;
        default: json << character; break;
        }
    }

    json << '"';
}

void appendRating(std::ostringstream& json, const ContentRating& rating)
{
    json << '{';

    appendQuoted(json, "source");
    json << ':';
    appendQuoted(json, sourceToString(rating.source()));

    json << ',';
    appendQuoted(json, "system");
    json << ':';
    appendQuoted(json, systemToString(rating.system()));

    json << ',';
    appendQuoted(json, "originalValue");
    json << ':';
    appendQuoted(json, rating.originalValue());

    json << ',';
    appendQuoted(json, "minimumAge");
    json << ':';
    json << rating.minimumAge();

    json << ',';
    appendQuoted(json, "confidence");
    json << ':';
    json << rating.confidence();

    json << ',';
    appendQuoted(json, "providerReference");
    json << ':';
    appendQuoted(json, rating.providerReference());

    json << '}';
}
}

std::string ContentRatingResolutionJsonSerializer::serialize(
    const ContentRatingResolutionResult& result) const
{
    std::ostringstream json;

    json << '{';

    appendQuoted(json, "resolved");
    json << ':';
    json << (result.isResolved() ? "true" : "false");

    json << ',';
    appendQuoted(json, "primaryRating");
    json << ':';

    if (result.isResolved())
    {
        appendRating(json, result.primaryRating());
    }
    else
    {
        json << "null";
    }

    json << ',';
    appendQuoted(json, "evidence");
    json << ':';
    json << '[';

    const std::vector<ContentRating>& evidence = result.evidence().all();

    for (std::size_t index = 0; index < evidence.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendRating(json, evidence[index]);
    }

    json << ']';

    json << '}';

    return json.str();
}
