#include "PersonQueryMatcher.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

std::string lowerCopy(
    const std::string& value)
{
    std::string result = value;
    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character));
        });
    return result;
}

bool containsCaseInsensitive(
    const std::string& value,
    const std::string& needle)
{
    return lowerCopy(value).find(
        lowerCopy(needle)) != std::string::npos;
}

}

bool PersonQueryMatcher::matches(
    const Person& person,
    const PersonQuery& query) const
{
    if (query.hasName()
        && !containsCaseInsensitive(
            person.originalName(),
            query.name())) {
        return false;
    }

    if (query.hasNormalizedName()
        && person.normalizedName() != query.normalizedName()) {
        return false;
    }

    if (query.hasRole()
        && person.role() != query.role()) {
        return false;
    }

    if (query.hasSource()
        && person.source() != query.source()) {
        return false;
    }

    if (query.hasProviderReference()
        && person.providerReference() != query.providerReference()) {
        return false;
    }

    return true;
}
