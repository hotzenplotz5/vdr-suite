#include "PersonQueryResultJsonSerializer.h"

#include <sstream>
#include <string>

namespace {

std::string sourceToString(ContentClassificationSource source)
{
    switch (source) {
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

std::string roleToString(PersonRole role)
{
    switch (role) {
    case PersonRole::Unknown:
        return "unknown";
    case PersonRole::Actor:
        return "actor";
    case PersonRole::Director:
        return "director";
    case PersonRole::Writer:
        return "writer";
    case PersonRole::Producer:
        return "producer";
    case PersonRole::Moderator:
        return "moderator";
    case PersonRole::Guest:
        return "guest";
    case PersonRole::Composer:
        return "composer";
    case PersonRole::Other:
        return "other";
    }

    return "unknown";
}

void appendQuoted(
    std::ostringstream& json,
    const std::string& value)
{
    json << '"';

    for (const char character : value) {
        switch (character) {
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
            if (static_cast<unsigned char>(character) < 0x20) {
                json << ' ';
            } else {
                json << character;
            }
            break;
        }
    }

    json << '"';
}

void appendKey(
    std::ostringstream& json,
    const std::string& key)
{
    appendQuoted(json, key);
    json << ':';
}

void appendPerson(
    std::ostringstream& json,
    const Person& person)
{
    json << '{';

    appendKey(json, "source");
    appendQuoted(json, sourceToString(person.source()));
    json << ',';

    appendKey(json, "role");
    appendQuoted(json, roleToString(person.role()));
    json << ',';

    appendKey(json, "originalName");
    appendQuoted(json, person.originalName());
    json << ',';

    appendKey(json, "normalizedName");
    appendQuoted(json, person.normalizedName());
    json << ',';

    appendKey(json, "characterName");
    appendQuoted(json, person.characterName());
    json << ',';

    appendKey(json, "confidence");
    json << person.confidence();
    json << ',';

    appendKey(json, "providerReference");
    appendQuoted(json, person.providerReference());

    json << '}';
}

}

std::string PersonQueryResultJsonSerializer::serialize(
    const PersonQueryResult& result) const
{
    std::ostringstream json;

    json << '{';

    appendKey(json, "totalCount");
    json << result.totalCount();
    json << ',';

    appendKey(json, "returnedCount");
    json << result.returnedCount();
    json << ',';

    appendKey(json, "limit");
    json << result.limit();
    json << ',';

    appendKey(json, "offset");
    json << result.offset();
    json << ',';

    appendKey(json, "persons");
    json << '[';

    const std::vector<Person>& persons = result.persons().all();

    for (std::size_t index = 0; index < persons.size(); ++index) {
        if (index > 0) {
            json << ',';
        }

        appendPerson(json, persons.at(index));
    }

    json << ']';
    json << '}';

    return json.str();
}
