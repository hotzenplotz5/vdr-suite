#pragma once

#include "Person.h"

#include <string>

class PersonQuery {
public:
    static PersonQuery createEmpty()
    {
        return PersonQuery();
    }

    static PersonQuery byName(
        const std::string& name)
    {
        PersonQuery query;
        query.name_ = name;
        return query;
    }

    static PersonQuery byNormalizedName(
        const std::string& normalizedName)
    {
        PersonQuery query;
        query.normalizedName_ = normalizedName;
        return query;
    }

    static PersonQuery byRole(
        PersonRole role)
    {
        PersonQuery query;
        query.role_ = role;
        query.hasRole_ = true;
        return query;
    }

    static PersonQuery bySource(
        ContentClassificationSource source)
    {
        PersonQuery query;
        query.source_ = source;
        query.hasSource_ = true;
        return query;
    }

    static PersonQuery byProviderReference(
        const std::string& providerReference)
    {
        PersonQuery query;
        query.providerReference_ = providerReference;
        return query;
    }

    PersonQuery& withRole(
        PersonRole role)
    {
        role_ = role;
        hasRole_ = true;
        return *this;
    }

    PersonQuery& withSource(
        ContentClassificationSource source)
    {
        source_ = source;
        hasSource_ = true;
        return *this;
    }

    PersonQuery& withName(
        const std::string& name)
    {
        name_ = name;
        return *this;
    }

    PersonQuery& withNormalizedName(
        const std::string& normalizedName)
    {
        normalizedName_ = normalizedName;
        return *this;
    }

    PersonQuery& withProviderReference(
        const std::string& providerReference)
    {
        providerReference_ = providerReference;
        return *this;
    }

    bool isEmpty() const
    {
        return !hasName()
            && !hasNormalizedName()
            && !hasRole()
            && !hasSource()
            && !hasProviderReference();
    }

    bool matchesAll() const
    {
        return isEmpty();
    }

    bool hasName() const
    {
        return !name_.empty();
    }

    const std::string& name() const
    {
        return name_;
    }

    bool hasNormalizedName() const
    {
        return !normalizedName_.empty();
    }

    const std::string& normalizedName() const
    {
        return normalizedName_;
    }

    bool hasRole() const
    {
        return hasRole_;
    }

    PersonRole role() const
    {
        return role_;
    }

    bool hasSource() const
    {
        return hasSource_;
    }

    ContentClassificationSource source() const
    {
        return source_;
    }

    bool hasProviderReference() const
    {
        return !providerReference_.empty();
    }

    const std::string& providerReference() const
    {
        return providerReference_;
    }

private:
    std::string name_;
    std::string normalizedName_;
    PersonRole role_ = PersonRole::Unknown;
    bool hasRole_ = false;
    ContentClassificationSource source_ = ContentClassificationSource::Derived;
    bool hasSource_ = false;
    std::string providerReference_;
};
