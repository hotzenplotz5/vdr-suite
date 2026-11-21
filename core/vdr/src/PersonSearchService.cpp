#include "PersonSearchService.h"

#include "PersonQueryMatcher.h"

#include <algorithm>

PersonQueryResult PersonSearchService::search(
    const PersonCollection& persons,
    const PersonQuery& query,
    int limit,
    int offset) const
{
    PersonQueryMatcher matcher;
    PersonCollection filteredPersons =
        PersonCollection::createEmpty();

    for (const auto& person : persons.all()) {
        if (matcher.matches(
            person,
            query)) {
            filteredPersons.add(person);
        }
    }

    const int totalCount = filteredPersons.size();
    const int normalizedOffset = std::max(
        0,
        offset);
    const int normalizedLimit = std::max(
        0,
        limit);

    PersonCollection page =
        PersonCollection::createEmpty();

    if (normalizedOffset < totalCount) {
        const int end = normalizedLimit > 0
            ? std::min(
                totalCount,
                normalizedOffset + normalizedLimit)
            : totalCount;

        for (int index = normalizedOffset; index < end; ++index) {
            page.add(
                filteredPersons.all().at(
                    static_cast<std::size_t>(index)));
        }
    }

    return PersonQueryResult::from(
        page,
        totalCount,
        normalizedLimit,
        normalizedOffset);
}
