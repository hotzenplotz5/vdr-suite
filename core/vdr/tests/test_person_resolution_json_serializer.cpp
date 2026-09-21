#include "PersonResolutionJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    PersonResolutionJsonSerializer serializer;

    std::string unresolvedJson =
        serializer.serialize(PersonResolutionResult::unresolved());

    assert(unresolvedJson.find("\"resolved\":false") != std::string::npos);
    assert(unresolvedJson.find("\"primaryPerson\":null") != std::string::npos);
    assert(unresolvedJson.find("\"evidence\":[]") != std::string::npos);

    PersonCollection persons = PersonCollection::createEmpty();

    persons.add(
        Person::from(
            ContentClassificationSource::Epg,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks"));

    persons.add(
        Person::withProviderReference(
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump",
            90,
            "tmdb:31"));

    PersonResolver resolver;
    std::string json =
        serializer.serialize(
            resolver.resolve(persons));

    assert(json.find("\"resolved\":true") != std::string::npos);
    assert(json.find("\"primaryPerson\"") != std::string::npos);
    assert(json.find("\"source\":\"tmdb\"") != std::string::npos);
    assert(json.find("\"source\":\"epg\"") != std::string::npos);
    assert(json.find("\"role\":\"actor\"") != std::string::npos);
    assert(json.find("\"originalName\":\"Tom Hanks\"") != std::string::npos);
    assert(json.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Forrest Gump\"") != std::string::npos);
    assert(json.find("\"confidence\":90") != std::string::npos);
    assert(json.find("\"providerReference\":\"tmdb:31\"") != std::string::npos);

    PersonCollection escapedPersons = PersonCollection::createEmpty();

    escapedPersons.add(
        Person::from(
            ContentClassificationSource::User,
            PersonRole::Actor,
            "Thomas \"Tom\" Hanks",
            "tom-hanks"));

    std::string escapedJson =
        serializer.serialize(
            resolver.resolve(escapedPersons));

    assert(escapedJson.find("Thomas \\\"Tom\\\" Hanks") != std::string::npos);

    std::cout << "test_person_resolution_json_serializer passed" << std::endl;
    return 0;
}
