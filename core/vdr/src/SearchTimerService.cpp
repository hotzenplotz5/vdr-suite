#include "SearchTimerService.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace {

bool containsText(
    const std::string& value,
    const std::string& text)
{
    return value.find(text) != std::string::npos;
}

bool matchesQuery(
    const SearchTimer& timer,
    const SearchTimerQuery& query)
{
    if (query.hasBackend()
        && timer.backendId() != query.backendId()) {
        return false;
    }

    if (query.hasState()
        && timer.state() != query.state()) {
        return false;
    }

    if (query.hasText()
        && !containsText(timer.name(), query.text())
        && !containsText(timer.query(), query.text())) {
        return false;
    }

    return true;
}

} // namespace

SearchTimerResult SearchTimerService::list(
    const std::vector<SearchTimer>& timers,
    const SearchTimerQuery& query) const
{
    std::vector<SearchTimer> matches;

    for (const SearchTimer& timer : timers) {
        if (matchesQuery(timer, query)) {
            matches.push_back(timer);
        }
    }

    const int totalCount =
        static_cast<int>(matches.size());
    const int normalizedOffset =
        std::max(
            0,
            query.offset());
    const int normalizedLimit =
        std::max(
            0,
            query.limit());

    std::vector<SearchTimer> page;

    if (normalizedOffset < totalCount) {
        const int end = normalizedLimit > 0
            ? std::min(
                totalCount,
                normalizedOffset + normalizedLimit)
            : totalCount;

        for (int index = normalizedOffset; index < end; ++index) {
            page.push_back(
                matches.at(
                    static_cast<std::size_t>(index)));
        }
    }

    return SearchTimerResult::from(
        page,
        totalCount,
        normalizedLimit,
        normalizedOffset);
}