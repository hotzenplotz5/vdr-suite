#include "PersonResolver.h"

#include <cassert>
#include <iostream>

int main()
{
    PersonResolver resolver;

    PersonResolutionResult unresolved =
        resolver.resolve(PersonCollection::createEmpty());

    assert(!unresolved.isResolved());
    assert(unresolved.evidence().empty());

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

    PersonResolutionResult resolved = resolver.resolve(persons);

    assert(resolved.isResolved());
    assert(resolved.primaryPerson().source() == ContentClassificationSource::Tmdb);
    assert(resolved.primaryPerson().role() == PersonRole::Actor);
    assert(resolved.primaryPerson().normalizedName() == "tom-hanks");
    assert(resolved.primaryPerson().hasCharacterName());
    assert(resolved.primaryPerson().characterName() == "Forrest Gump");
    assert(resolved.primaryPerson().hasProviderReference());
    assert(resolved.evidence().size() == 2);

    PersonCollection userOverride = PersonCollection::createEmpty();

    userOverride.add(
        Person::withProviderReference(
            ContentClassificationSource::Tmdb,
            PersonRole::Actor,
            "Tom Hanks",
            "tom-hanks",
            "Forrest Gump",
            80,
            "tmdb:31"));

    userOverride.add(
        Person::from(
            ContentClassificationSource::User,
            PersonRole::Actor,
            "Thomas Hanks",
            "tom-hanks"));

    PersonResolutionResult manual = resolver.resolve(userOverride);

    assert(manual.isResolved());
    assert(manual.primaryPerson().source() == ContentClassificationSource::User);
    assert(manual.primaryPerson().originalName() == "Thomas Hanks");

    PersonCollection providerReferenceTie = PersonCollection::createEmpty();

    providerReferenceTie.add(
        Person::from(
            ContentClassificationSource::Tmdb,
            PersonRole::Director,
            "Christopher Nolan",
            "christopher-nolan"));

    providerReferenceTie.add(
        Person::withProviderReference(
            ContentClassificationSource::Tvdb,
            PersonRole::Director,
            "Christopher Nolan",
            "christopher-nolan",
            "",
            0,
            "tvdb:director:123"));

    PersonResolutionResult tied = resolver.resolve(providerReferenceTie);

    assert(tied.isResolved());
    assert(tied.primaryPerson().hasProviderReference());

    std::cout << "test_person_resolver passed" << std::endl;
    return 0;
}
