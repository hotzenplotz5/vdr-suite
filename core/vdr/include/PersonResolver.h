#pragma once

#include "Person.h"

class PersonResolutionResult
{
public:
    static PersonResolutionResult unresolved()
    {
        return PersonResolutionResult();
    }

    static PersonResolutionResult resolved(
        const Person& primaryPerson,
        const PersonCollection& evidence)
    {
        PersonResolutionResult result;
        result.resolved_ = true;
        result.primaryPerson_ = primaryPerson;
        result.evidence_ = evidence;
        return result;
    }

    bool isResolved() const
    {
        return resolved_;
    }

    const Person& primaryPerson() const
    {
        return primaryPerson_;
    }

    const PersonCollection& evidence() const
    {
        return evidence_;
    }

private:
    bool resolved_ = false;
    Person primaryPerson_ =
        Person::from(
            ContentClassificationSource::Derived,
            PersonRole::Unknown,
            "",
            "");
    PersonCollection evidence_ =
        PersonCollection::createEmpty();
};

class PersonResolver
{
public:
    PersonResolutionResult resolve(
        const PersonCollection& persons) const
    {
        if (persons.empty())
        {
            return PersonResolutionResult::unresolved();
        }

        const Person* selected = nullptr;

        for (const Person& person : persons.all())
        {
            if (selected == nullptr || isBetter(person, *selected))
            {
                selected = &person;
            }
        }

        return PersonResolutionResult::resolved(
            *selected,
            persons);
    }

private:
    bool isBetter(
        const Person& candidate,
        const Person& current) const
    {
        if (candidate.source() == ContentClassificationSource::User &&
            current.source() != ContentClassificationSource::User)
        {
            return true;
        }

        if (candidate.source() != ContentClassificationSource::User &&
            current.source() == ContentClassificationSource::User)
        {
            return false;
        }

        if (candidate.hasConfidence() != current.hasConfidence())
        {
            return candidate.hasConfidence();
        }

        if (candidate.confidence() != current.confidence())
        {
            return candidate.confidence() > current.confidence();
        }

        if (candidate.hasProviderReference() != current.hasProviderReference())
        {
            return candidate.hasProviderReference();
        }

        return sourcePriority(candidate.source()) > sourcePriority(current.source());
    }

    int sourcePriority(
        ContentClassificationSource source) const
    {
        switch (source)
        {
        case ContentClassificationSource::User:
            return 100;
        case ContentClassificationSource::Tmdb:
            return 90;
        case ContentClassificationSource::Tvdb:
            return 80;
        case ContentClassificationSource::Imdb:
            return 70;
        case ContentClassificationSource::Tvscraper:
            return 60;
        case ContentClassificationSource::Scraper2Vdr:
            return 50;
        case ContentClassificationSource::Dvb:
            return 40;
        case ContentClassificationSource::Epg:
            return 30;
        case ContentClassificationSource::Folder:
            return 20;
        case ContentClassificationSource::Derived:
            return 10;
        }

        return 0;
    }
};
