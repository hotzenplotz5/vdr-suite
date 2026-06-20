#include "Person.h"

#include <cassert>
#include <iostream>

int main()
{
    Person actor =
        Person::withCharacterName(
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump");

    assert(actor.source() == ContentClassificationSource::Tmdb);
    assert(actor.role() == PersonRole::Actor);
    assert(actor.originalName() == "Tom Hanks");
    assert(actor.normalizedName() == "tom-hanks");
    assert(actor.hasCharacterName());
    assert(actor.characterName() == "Forrest Gump");
    assert(!actor.hasConfidence());
    assert(!actor.hasProviderReference());

    Person director =
        Person::withProviderReference(
            ContentClassificationSource::Imdb,
            PersonRole::Director,
            "Christopher Nolan",
            "christopher-nolan",
            "",
            90,
            "imdb:nm0634240");

    assert(director.source() == ContentClassificationSource::Imdb);
    assert(director.role() == PersonRole::Director);
    assert(!director.hasCharacterName());
    assert(director.hasConfidence());
    assert(director.confidence() == 90);
    assert(director.hasProviderReference());
    assert(director.providerReference() == "imdb:nm0634240");

    PersonCollection collection = PersonCollection::createEmpty();

    assert(collection.empty());
    assert(collection.size() == 0);

    collection.add(actor);
    collection.add(director);

    assert(!collection.empty());
    assert(collection.size() == 2);
    assert(collection.hasRole(PersonRole::Actor));
    assert(collection.hasRole(PersonRole::Director));
    assert(!collection.hasRole(PersonRole::Producer));
    assert(collection.hasNormalizedName("tom-hanks"));
    assert(collection.hasNormalizedName("christopher-nolan"));
    assert(!collection.hasNormalizedName("steven-spielberg"));

    std::cout << "test_person passed" << std::endl;
    return 0;
}
