#include "PersonSearchService.h"

#include <cassert>
#include <iostream>

namespace {

Person makeActor(
    const std::string& originalName,
    const std::string& normalizedName,
    const std::string& providerReference)
{
    return Person::withProviderReference(
        ContentClassificationSource::Tmdb,
        PersonRole::Actor,
        originalName,
        normalizedName,
        "",
        90,
        providerReference);
}

Person makeDirector(
    const std::string& originalName,
    const std::string& normalizedName,
    const std::string& providerReference)
{
    return Person::withProviderReference(
        ContentClassificationSource::Imdb,
        PersonRole::Director,
        originalName,
        normalizedName,
        "",
        85,
        providerReference);
}

}

int main()
{
    PersonCollection persons = PersonCollection::createEmpty();
    persons.add(
        makeActor(
            "Tom Hanks",
            "tom-hanks",
            "tmdb:31"));
    persons.add(
        makeActor(
            "Meg Ryan",
            "meg-ryan",
            "tmdb:5344"));
    persons.add(
        makeDirector(
            "Steven Spielberg",
            "steven-spielberg",
            "imdb:nm0000229"));

    PersonSearchService service;

    PersonQueryResult all = service.search(
        persons,
        PersonQuery::createEmpty(),
        10,
        0);

    assert(all.totalCount() == 3);
    assert(all.returnedCount() == 3);
    assert(all.limit() == 10);
    assert(all.offset() == 0);

    PersonQueryResult actors = service.search(
        persons,
        PersonQuery::byRole(PersonRole::Actor),
        10,
        0);

    assert(actors.totalCount() == 2);
    assert(actors.returnedCount() == 2);
    assert(actors.persons().all().at(0).normalizedName() == "tom-hanks");
    assert(actors.persons().all().at(1).normalizedName() == "meg-ryan");

    PersonQueryResult directors = service.search(
        persons,
        PersonQuery::byRole(PersonRole::Director),
        10,
        0);

    assert(directors.totalCount() == 1);
    assert(directors.returnedCount() == 1);
    assert(directors.persons().all().at(0).normalizedName()
        == "steven-spielberg");

    PersonQueryResult normalized = service.search(
        persons,
        PersonQuery::byNormalizedName("tom-hanks"),
        10,
        0);

    assert(normalized.totalCount() == 1);
    assert(normalized.returnedCount() == 1);
    assert(normalized.persons().all().at(0).providerReference()
        == "tmdb:31");

    PersonQueryResult provider = service.search(
        persons,
        PersonQuery::byProviderReference("imdb:nm0000229"),
        10,
        0);

    assert(provider.totalCount() == 1);
    assert(provider.returnedCount() == 1);
    assert(provider.persons().all().at(0).role()
        == PersonRole::Director);

    PersonQueryResult firstPage = service.search(
        persons,
        PersonQuery::createEmpty(),
        2,
        0);

    assert(firstPage.totalCount() == 3);
    assert(firstPage.returnedCount() == 2);
    assert(firstPage.persons().all().at(0).normalizedName() == "tom-hanks");
    assert(firstPage.persons().all().at(1).normalizedName() == "meg-ryan");

    PersonQueryResult secondPage = service.search(
        persons,
        PersonQuery::createEmpty(),
        2,
        2);

    assert(secondPage.totalCount() == 3);
    assert(secondPage.returnedCount() == 1);
    assert(secondPage.persons().all().at(0).normalizedName()
        == "steven-spielberg");

    PersonQueryResult unlimited = service.search(
        persons,
        PersonQuery::createEmpty(),
        0,
        1);

    assert(unlimited.totalCount() == 3);
    assert(unlimited.returnedCount() == 2);
    assert(unlimited.persons().all().at(0).normalizedName() == "meg-ryan");
    assert(unlimited.persons().all().at(1).normalizedName()
        == "steven-spielberg");

    PersonQueryResult outOfRange = service.search(
        persons,
        PersonQuery::createEmpty(),
        10,
        20);

    assert(outOfRange.totalCount() == 3);
    assert(outOfRange.returnedCount() == 0);
    assert(outOfRange.offset() == 20);

    PersonQueryResult negativePaging = service.search(
        persons,
        PersonQuery::createEmpty(),
        -1,
        -5);

    assert(negativePaging.totalCount() == 3);
    assert(negativePaging.returnedCount() == 3);
    assert(negativePaging.limit() == 0);
    assert(negativePaging.offset() == 0);

    std::cout << "test_person_search_service passed" << std::endl;
    return 0;
}
