#include "RecordingPersonSearchService.h"

#include "PersonQueryMatcher.h"

#include <algorithm>
#include <cstddef>
#include <vector>

RecordingPersonSearchResult RecordingPersonSearchService::search(
    const std::vector<VdrRecording>& recordings,
    const PersonQuery& query,
    int limit,
    int offset) const
{
    PersonQueryMatcher matcher;
    std::vector<RecordingPersonSearchMatch> matches;

    for (const VdrRecording& recording : recordings) {
        for (const Person& person : recording.persons.all()) {
            if (matcher.matches(
                person,
                query)) {
                matches.emplace_back(
                    recording,
                    person);
            }
        }
    }

    const int totalCount =
        static_cast<int>(matches.size());
    const int normalizedOffset =
        std::max(
            0,
            offset);
    const int normalizedLimit =
        std::max(
            0,
            limit);

    std::vector<RecordingPersonSearchMatch> page;

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

    return RecordingPersonSearchResult::from(
        page,
        totalCount,
        normalizedLimit,
        normalizedOffset);
}
