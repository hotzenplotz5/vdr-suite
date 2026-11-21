#include "VdrRecordingQueryMatcher.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
std::string lowerCopy(
    const std::string& value)
{
    std::string result = value;

    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    return result;
}

// Same year/date validation as Home Recent Movies. Filter before pagination;
// the browser retains its locale-aware ordering and final defensive predicate.
int releaseYear(const std::string& input)
{
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return 0;
    const auto last = input.find_last_not_of(" \t\r\n");
    const std::string value = input.substr(first, last - first + 1);
    if (value.size() != 4 && value.size() != 10) return 0;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (i == 4 || i == 7) { if (value[i] != '-') return 0; }
        else if (value[i] < '0' || value[i] > '9') return 0;
    }
    const int year = std::stoi(value.substr(0, 4));
    if (year < 1000) return 0;
    if (value.size() == 4) return year;
    const int month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    if (month < 1 || month > 12) return 0;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return day > 0 && day <= days[month - 1] ? year : 0;
}

bool containsCaseInsensitive(
    const std::string& value,
    const std::string& needle)
{
    return lowerCopy(value).find(
               lowerCopy(needle)) != std::string::npos;
}
}

bool VdrRecordingQueryMatcher::matches(
    const VdrRecording& recording,
    const VdrRecordingQuery& query) const
{
    if (query.movieReleaseYearFrom() > 0 || query.movieReleaseYearTo() > 0)
    {
        const int year = releaseYear(recording.metadata.provider.releaseDate);
        if (recording.metadata.provider.contentKind != VdrRecordingContentKind::Movie ||
            year == 0 ||
            (query.movieReleaseYearFrom() > 0 && year < query.movieReleaseYearFrom()) ||
            (query.movieReleaseYearTo() > 0 && year > query.movieReleaseYearTo())) return false;
    }

    if (query.hasTitleFilter() &&
        !containsCaseInsensitive(
            recording.title,
            query.titleFilter()))
    {
        return false;
    }

    if (query.hasPathFilter() &&
        !containsCaseInsensitive(
            recording.path,
            query.pathFilter()))
    {
        return false;
    }

    if (query.hasBackendFilter())
    {
        const bool legacyDefaultMatch =
            query.backendFilter() == "default" &&
            recording.backendId.empty();

        if (!legacyDefaultMatch &&
            recording.backendId != query.backendFilter())
        {
            return false;
        }
    }

    if (query.hasFromStartTime() &&
        recording.startTime < query.fromStartTime())
    {
        return false;
    }

    if (query.hasToStartTime() &&
        recording.startTime > query.toStartTime())
    {
        return false;
    }

    if (query.hasMinDurationSeconds() &&
        recording.durationSeconds < query.minDurationSeconds())
    {
        return false;
    }

    if (query.hasMaxDurationSeconds() &&
        recording.durationSeconds > query.maxDurationSeconds())
    {
        return false;
    }

    return true;
}
