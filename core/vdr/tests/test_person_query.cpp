#include "PersonQuery.h"

#include <cassert>
#include <iostream>

int main()
{
    PersonQuery emptyQuery = PersonQuery::createEmpty();
    assert(emptyQuery.isEmpty());
    assert(emptyQuery.matchesAll());
    assert(!emptyQuery.hasName());
    assert(!emptyQuery.hasNormalizedName());
    assert(!emptyQuery.hasRole());
    assert(!emptyQuery.hasSource());
    assert(!emptyQuery.hasProviderReference());

    PersonQuery actorQuery = PersonQuery::byRole(PersonRole::Actor);
    assert(!actorQuery.isEmpty());
    assert(!actorQuery.matchesAll());
    assert(actorQuery.hasRole());
    assert(actorQuery.role() == PersonRole::Actor);

    PersonQuery directorQuery = PersonQuery::createEmpty()
        .withRole(PersonRole::Director);
    assert(directorQuery.hasRole());
    assert(directorQuery.role() == PersonRole::Director);

    PersonQuery normalizedNameQuery =
        PersonQuery::byNormalizedName("tom-hanks");
    assert(normalizedNameQuery.hasNormalizedName());
    assert(normalizedNameQuery.normalizedName() == "tom-hanks");

    PersonQuery characterNameQuery =
        PersonQuery::byCharacterName("ace ventura");
    assert(characterNameQuery.hasCharacterName());
    assert(characterNameQuery.characterName() == "ace ventura");

    PersonQuery providerReferenceQuery =
        PersonQuery::byProviderReference("tmdb:31");
    assert(providerReferenceQuery.hasProviderReference());
    assert(providerReferenceQuery.providerReference() == "tmdb:31");

    PersonQuery combinedQuery = PersonQuery::byName("Tom Hanks")
        .withNormalizedName("tom-hanks")
        .withCharacterName("Forrest Gump")
        .withRole(PersonRole::Actor)
        .withSource(ContentClassificationSource::Tmdb)
        .withProviderReference("tmdb:31");

    assert(combinedQuery.hasName());
    assert(combinedQuery.name() == "Tom Hanks");
    assert(combinedQuery.hasNormalizedName());
    assert(combinedQuery.normalizedName() == "tom-hanks");
    assert(combinedQuery.hasCharacterName());
    assert(combinedQuery.characterName() == "Forrest Gump");
    assert(combinedQuery.hasRole());
    assert(combinedQuery.role() == PersonRole::Actor);
    assert(combinedQuery.hasSource());
    assert(combinedQuery.source() == ContentClassificationSource::Tmdb);
    assert(combinedQuery.hasProviderReference());
    assert(combinedQuery.providerReference() == "tmdb:31");

    std::cout << "test_person_query passed" << std::endl;
    return 0;
}
