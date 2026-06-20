#pragma once

#include "GenreClassification.h"

#include <string>
#include <vector>

enum class PersonRole
{
    Unknown,
    Actor,
    Director,
    Writer,
    Producer,
    Moderator,
    Guest,
    Composer,
    Other
};

class Person
{
public:
    static Person from(
        ContentClassificationSource source,
        PersonRole role,
        const std::string& originalName,
        const std::string& normalizedName)
    {
        return Person(
            source,
            role,
            originalName,
            normalizedName,
            "",
            0,
            "");
    }

    static Person withCharacterName(
        ContentClassificationSource source,
        PersonRole role,
        const std::string& originalName,
        const std::string& normalizedName,
        const std::string& characterName)
    {
        return Person(
            source,
            role,
            originalName,
            normalizedName,
            characterName,
            0,
            "");
    }

    static Person withProviderReference(
        ContentClassificationSource source,
        PersonRole role,
        const std::string& originalName,
        const std::string& normalizedName,
        const std::string& characterName,
        int confidence,
        const std::string& providerReference)
    {
        return Person(
            source,
            role,
            originalName,
            normalizedName,
            characterName,
            confidence,
            providerReference);
    }

    ContentClassificationSource source() const
    {
        return source_;
    }

    PersonRole role() const
    {
        return role_;
    }

    const std::string& originalName() const
    {
        return originalName_;
    }

    const std::string& normalizedName() const
    {
        return normalizedName_;
    }

    const std::string& characterName() const
    {
        return characterName_;
    }

    bool hasCharacterName() const
    {
        return !characterName_.empty();
    }

    int confidence() const
    {
        return confidence_;
    }

    bool hasConfidence() const
    {
        return confidence_ > 0;
    }

    const std::string& providerReference() const
    {
        return providerReference_;
    }

    bool hasProviderReference() const
    {
        return !providerReference_.empty();
    }

private:
    Person(
        ContentClassificationSource source,
        PersonRole role,
        const std::string& originalName,
        const std::string& normalizedName,
        const std::string& characterName,
        int confidence,
        const std::string& providerReference)
        : source_(source),
          role_(role),
          originalName_(originalName),
          normalizedName_(normalizedName),
          characterName_(characterName),
          confidence_(confidence),
          providerReference_(providerReference)
    {
    }

    ContentClassificationSource source_;
    PersonRole role_;
    std::string originalName_;
    std::string normalizedName_;
    std::string characterName_;
    int confidence_ = 0;
    std::string providerReference_;
};

class PersonCollection
{
public:
    static PersonCollection createEmpty()
    {
        return PersonCollection();
    }

    void add(
        const Person& person)
    {
        persons_.push_back(person);
    }

    const std::vector<Person>& all() const
    {
        return persons_;
    }

    bool empty() const
    {
        return persons_.empty();
    }

    int size() const
    {
        return static_cast<int>(persons_.size());
    }

    bool hasRole(
        PersonRole role) const
    {
        for (const Person& person : persons_)
        {
            if (person.role() == role)
            {
                return true;
            }
        }

        return false;
    }

    bool hasNormalizedName(
        const std::string& normalizedName) const
    {
        for (const Person& person : persons_)
        {
            if (person.normalizedName() == normalizedName)
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<Person> persons_;
};
