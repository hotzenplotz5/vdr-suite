#include "PersonController.h"

#include "PersonQueryResultJsonSerializer.h"
#include "PersonResolutionJsonSerializer.h"
#include "PersonSearchService.h"

#include <cassert>
#include <iostream>
#include <string>

namespace {

PersonCollection makePersons()
{
    PersonCollection persons =
        PersonCollection::createEmpty();

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
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Meg Ryan",
            "meg-ryan",
            "",
            90,
            "tmdb:5344"));

    persons.add(
        Person::withProviderReference(
            ContentClassificationSource::Imdb,
            PersonRole::Director,
            "Steven Spielberg",
            "steven-spielberg",
            "",
            85,
            "imdb:nm0000229"));

    return persons;
}

}

int main()
{
    PersonResolutionJsonSerializer resolutionSerializer;
    PersonSearchService searchService;
    PersonQueryResultJsonSerializer queryResultSerializer;

    PersonController controller(
        resolutionSerializer,
        searchService,
        queryResultSerializer);

    const PersonCollection persons =
        makePersons();

    ApiResponse allResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "",
            10,
            0);

    assert(allResponse.statusCode == 200);
    assert(allResponse.contentType == "application/json");
    assert(allResponse.body.find("\"totalCount\":3") != std::string::npos);
    assert(allResponse.body.find("\"returnedCount\":3") != std::string::npos);

    ApiResponse actorResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "actor",
            "",
            "",
            10,
            0);

    assert(actorResponse.statusCode == 200);
    assert(actorResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(actorResponse.body.find("\"role\":\"actor\"") != std::string::npos);
    assert(actorResponse.body.find("\"normalizedName\":\"tom-hanks\"") != std::string::npos);
    assert(actorResponse.body.find("\"normalizedName\":\"meg-ryan\"") != std::string::npos);

    ApiResponse directorResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "director",
            "",
            "",
            10,
            0);

    assert(directorResponse.statusCode == 200);
    assert(directorResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(directorResponse.body.find("\"normalizedName\":\"steven-spielberg\"") != std::string::npos);

    ApiResponse normalizedNameResponse =
        controller.searchPersons(
            persons,
            "",
            "tom-hanks",
            "",
            "",
            "",
            10,
            0);

    assert(normalizedNameResponse.statusCode == 200);
    assert(normalizedNameResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(normalizedNameResponse.body.find("\"providerReference\":\"tmdb:31\"") != std::string::npos);

    ApiResponse sourceResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "imdb",
            "",
            10,
            0);

    assert(sourceResponse.statusCode == 200);
    assert(sourceResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(sourceResponse.body.find("\"source\":\"imdb\"") != std::string::npos);

    ApiResponse providerResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "tmdb:5344",
            10,
            0);

    assert(providerResponse.statusCode == 200);
    assert(providerResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(providerResponse.body.find("\"normalizedName\":\"meg-ryan\"") != std::string::npos);

    ApiResponse pagedResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "",
            1,
            1);

    assert(pagedResponse.statusCode == 200);
    assert(pagedResponse.body.find("\"totalCount\":3") != std::string::npos);
    assert(pagedResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(pagedResponse.body.find("\"limit\":1") != std::string::npos);
    assert(pagedResponse.body.find("\"offset\":1") != std::string::npos);

    ApiResponse invalidRoleResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "hero",
            "",
            "",
            10,
            0);

    assert(invalidRoleResponse.statusCode == 400);
    assert(invalidRoleResponse.body.find("invalid person role") != std::string::npos);

    ApiResponse invalidSourceResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "provider-x",
            "",
            10,
            0);

    assert(invalidSourceResponse.statusCode == 400);
    assert(invalidSourceResponse.body.find("invalid person source") != std::string::npos);

    ApiResponse negativeLimitResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "",
            -1,
            0);

    assert(negativeLimitResponse.statusCode == 400);
    assert(negativeLimitResponse.body.find("limit must not be negative") != std::string::npos);

    ApiResponse negativeOffsetResponse =
        controller.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "",
            10,
            -1);

    assert(negativeOffsetResponse.statusCode == 400);
    assert(negativeOffsetResponse.body.find("offset must not be negative") != std::string::npos);

    PersonController unavailableController(
        resolutionSerializer);

    ApiResponse unavailableResponse =
        unavailableController.searchPersons(
            persons,
            "",
            "",
            "",
            "",
            "",
            10,
            0);

    assert(unavailableResponse.statusCode == 503);
    assert(unavailableResponse.body.find("person search unavailable") != std::string::npos);

    std::cout << "test_person_query_controller passed" << std::endl;
    return 0;
}
