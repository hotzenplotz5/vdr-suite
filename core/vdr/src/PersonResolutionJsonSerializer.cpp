#include "PersonResolutionJsonSerializer.h"

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

std::string roleToString(PersonRole role)
{
    switch (role)
    {
    case PersonRole::Unknown: return "unknown";
    case PersonRole::Actor: return "actor";
    case PersonRole::Director: return "director";
    case PersonRole::Writer: return "writer";
    case PersonRole::Producer: return "producer";
    case PersonRole::Moderator: return "moderator";
    case PersonRole::Guest: return "guest";
    case PersonRole::Composer: return "composer";
    case PersonRole::Other: return "other";
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

void appendPerson(std::ostringstream& json, const Person& person)
{
    json << '{';

    appendQuoted(json, "source");
    json << ':';
    appendQuoted(json, sourceToString(person.source()));

    json << ',';
    appendQuoted(json, "role");
    json << ':';
    appendQuoted(json, roleToString(person.role()));

    json << ',';
    appendQuoted(json, "originalName");
    json << ':';
    appendQuoted(json, person.originalName());

    json << ',';
    appendQuoted(json, "normalizedName");
    json << ':';
    appendQuoted(json, person.normalizedName());

    json << ',';
    appendQuoted(json, "characterName");
    json << ':';
    appendQuoted(json, person.characterName());

    json << ',';
    appendQuoted(json, "confidence");
    json << ':';
    json << person.confidence();

    json << ',';
    appendQuoted(json, "providerReference");
    json << ':';
    appendQuoted(json, person.providerReference());

    json << '}';
}
}

std::string PersonResolutionJsonSerializer::serialize(
    const PersonResolutionResult& result) const
{
    std::ostringstream json;

    json << '{';

    appendQuoted(json, "resolved");
    json << ':';
    json << (result.isResolved() ? "true" : "false");

    json << ',';
    appendQuoted(json, "primaryPerson");
    json << ':';

    if (result.isResolved())
    {
        appendPerson(json, result.primaryPerson());
    }
    else
    {
        json << "null";
    }

    json << ',';
    appendQuoted(json, "evidence");
    json << ':';
    json << '[';

    const std::vector<Person>& evidence = result.evidence().all();

    for (std::size_t index = 0; index < evidence.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        appendPerson(json, evidence[index]);
    }

    json << ']';

    json << '}';

    return json.str();
}
