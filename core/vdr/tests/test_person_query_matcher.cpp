#include "PersonQueryMatcher.h"

#include <cassert>
#include <iostream>

int main()
{
    const Person actor = Person::withProviderReference(
        ContentClassificationSource::Tmdb,
        PersonRole::Actor,
        "Tom Hanks",
        "tom-hanks",
        "Forrest Gump",
        95,
        "tmdb:31");

    const Person director = Person::withProviderReference(
        ContentClassificationSource::Imdb,
        PersonRole::Director,
        "Steven Spielberg",
        "steven-spielberg",
        "",
        90,
        "imdb:nm0000229");

    PersonQueryMatcher matcher;

    assert(matcher.matches(
        actor,
        PersonQuery::createEmpty()));

    assert(matcher.matches(
        actor,
        PersonQuery::byRole(PersonRole::Actor)));

    assert(!matcher.matches(
        actor,
        PersonQuery::byRole(PersonRole::Director)));

    assert(matcher.matches(
        director,
        PersonQuery::byRole(PersonRole::Director)));

    assert(matcher.matches(
        actor,
        PersonQuery::byName("tom")));

    assert(matcher.matches(
        actor,
        PersonQuery::byName("HANKS")));

    assert(!matcher.matches(
        actor,
        PersonQuery::byName("Spielberg")));

    assert(matcher.matches(
        actor,
        PersonQuery::byNormalizedName("tom-hanks")));

    assert(!matcher.matches(
        actor,
        PersonQuery::byNormalizedName("tom")));

    assert(matcher.matches(
        actor,
        PersonQuery::bySource(ContentClassificationSource::Tmdb)));

    assert(!matcher.matches(
        actor,
        PersonQuery::bySource(ContentClassificationSource::Imdb)));

    assert(matcher.matches(
        actor,
        PersonQuery::byProviderReference("tmdb:31")));

    assert(!matcher.matches(
        actor,
        PersonQuery::byProviderReference("imdb:nm0000229")));

    PersonQuery combined = PersonQuery::byName("tom")
        .withNormalizedName("tom-hanks")
        .withRole(PersonRole::Actor)
        .withSource(ContentClassificationSource::Tmdb)
        .withProviderReference("tmdb:31");

    assert(matcher.matches(
        actor,
        combined));

    combined.withProviderReference("tmdb:999");
    assert(!matcher.matches(
        actor,
        combined));

    std::cout << "test_person_query_matcher passed" << std::endl;
    return 0;
}
