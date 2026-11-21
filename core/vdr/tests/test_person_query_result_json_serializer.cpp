#include "PersonQueryResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    PersonQueryResultJsonSerializer serializer;

    std::string emptyJson = serializer.serialize(
        PersonQueryResult::empty(
            25,
            0));

    assert(emptyJson.find("\"totalCount\":0") != std::string::npos);
    assert(emptyJson.find("\"returnedCount\":0") != std::string::npos);
    assert(emptyJson.find("\"limit\":25") != std::string::npos);
    assert(emptyJson.find("\"offset\":0") != std::string::npos);
    assert(emptyJson.find("\"persons\":[]") != std::string::npos);

    PersonCollection persons = PersonCollection::createEmpty();
    persons.add(
        Person::withProviderReference(
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump",
            95,
            "tmdb:31"));
    persons.add(
        Person::withProviderReference(
            ContentClassificationSource::Imdb,
            PersonRole::Director,
            "Steven Spielberg",
            "steven-spielberg",
            "",
            90,
            "imdb:nm0000229"));

    std::string json = serializer.serialize(
        PersonQueryResult::from(
            persons,
            7,
            2,
            4));

    assert(json.find("\"totalCount\":7") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":4") != std::string::npos);
    assert(json.find("\"persons\":[") != std::string::npos);

    assert(json.find("\"source\":\"tmdb\"") != std::string::npos);
    assert(json.find("\"role\":\"actor\"") != std::string::npos);
    assert(json.find("\"originalName\":\"Tom Hanks\"") != std::string::npos);
    assert(json.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);
    assert(json.find("\"characterName\":\"Forrest Gump\"") != std::string::npos);
    assert(json.find("\"confidence\":95") != std::string::npos);
    assert(json.find("\"providerReference\":\"tmdb:31\"") != std::string::npos);

    assert(json.find("\"source\":\"imdb\"") != std::string::npos);
    assert(json.find("\"role\":\"director\"") != std::string::npos);
    assert(json.find("\"originalName\":\"Steven Spielberg\"") != std::string::npos);
    assert(json.find("\"providerReference\":\"imdb:nm0000229\"") != std::string::npos);

    PersonCollection escapedPersons = PersonCollection::createEmpty();
    escapedPersons.add(
        Person::from(
            ContentClassificationSource::User,
            PersonRole::Actor,
            "Thomas \"Tom\" Hanks",
            "tom-hanks"));

    std::string escapedJson = serializer.serialize(
        PersonQueryResult::from(
            escapedPersons,
            1,
            10,
            0));

    assert(escapedJson.find("Thomas \\\"Tom\\\" Hanks") != std::string::npos);

    std::cout << "test_person_query_result_json_serializer passed" << std::endl;
    return 0;
}
