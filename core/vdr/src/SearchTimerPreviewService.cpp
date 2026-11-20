#include "SearchTimerPreviewService.h"

#include "EpgSearchMatch.h"
#include "EpgSearchRequest.h"
#include "EpgSearchRequestMapper.h"
#include "EpgSearchResult.h"
#include "EpgSearchService.h"
#include "SearchTimerPreviewEpgInputContext.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

constexpr int previewTypoFallbackTolerance = 2;
constexpr int fallbackDescriptionPenalty = 20;
constexpr int fallbackSubtitlePenalty = 10;
constexpr int fallbackNoMatchRank = 1000000;

bool hasExplicitComparisonOptions(
    const SearchTimer& searchTimer)
{
    return searchTimer.comparisonOptions().compareTitle() ||
           searchTimer.comparisonOptions().compareSubtitle() ||
           searchTimer.comparisonOptions().compareSummary();
}

EpgSearchMode searchModeForSearchTimerMode(
    int mode)
{
    switch (mode)
    {
    case 1:
        return EpgSearchMode::AllWords;
    case 2:
        return EpgSearchMode::AnyWord;
    case 3:
        return EpgSearchMode::Exact;
    case 4:
        return EpgSearchMode::RegularExpression;
    case 5:
        return EpgSearchMode::Fuzzy;
    default:
        return EpgSearchMode::Phrase;
    }
}

bool shouldApplyPreviewTypoFallback(
    const SearchTimer& searchTimer,
    const EpgSearchResult& result)
{
    return result.totalCount() == 0 &&
           !searchTimer.query().empty() &&
           searchTimer.matchOptions().mode() == 0;
}

std::string normalizedWord(
    const std::string& value)
{
    std::string normalized;

    for (const unsigned char character : value)
    {
        if (std::isalnum(character))
        {
            normalized.push_back(
                static_cast<char>(std::tolower(character)));
        }
    }

    return normalized;
}

std::vector<std::string> splitNormalizedWords(
    const std::string& value)
{
    std::vector<std::string> words;
    std::stringstream stream(value);
    std::string word;

    while (stream >> word)
    {
        const std::string normalized = normalizedWord(word);
        if (!normalized.empty())
        {
            words.push_back(normalized);
        }
    }

    return words;
}

int levenshteinDistance(
    const std::string& left,
    const std::string& right)
{
    std::vector<int> previous(right.size() + 1);
    std::vector<int> current(right.size() + 1);

    for (std::size_t column = 0; column <= right.size(); ++column)
    {
        previous[column] = static_cast<int>(column);
    }

    for (std::size_t row = 1; row <= left.size(); ++row)
    {
        current[0] = static_cast<int>(row);

        for (std::size_t column = 1; column <= right.size(); ++column)
        {
            const int cost =
                left[row - 1] == right[column - 1] ? 0 : 1;

            current[column] =
                std::min(
                    std::min(
                        previous[column] + 1,
                        current[column - 1] + 1),
                    previous[column - 1] + cost);
        }

        previous.swap(current);
    }

    return previous[right.size()];
}

int bestFieldDistance(
    const std::string& queryWord,
    const std::string& field)
{
    int best = fallbackNoMatchRank;

    for (const std::string& fieldWord : splitNormalizedWords(field))
    {
        best = std::min(
            best,
            levenshteinDistance(fieldWord, queryWord));
    }

    return best;
}

int fallbackRankForEvent(
    const std::string& query,
    const VdrEvent& event)
{
    const std::vector<std::string> queryWords =
        splitNormalizedWords(query);

    if (queryWords.empty())
    {
        return fallbackNoMatchRank;
    }

    int rank = 0;

    for (const std::string& queryWord : queryWords)
    {
        const int titleRank =
            bestFieldDistance(queryWord, event.title);
        const int subtitleRank =
            bestFieldDistance(queryWord, event.subtitle) +
            fallbackSubtitlePenalty;
        const int descriptionRank =
            bestFieldDistance(queryWord, event.description) +
            fallbackDescriptionPenalty;

        rank += std::min(
            titleRank,
            std::min(
                subtitleRank,
                descriptionRank));
    }

    return rank;
}

EpgSearchResult rankTypoFallbackResult(
    const SearchTimer& searchTimer,
    const EpgSearchResult& result)
{
    std::vector<EpgSearchMatch> rankedMatches = result.matches();

    std::stable_sort(
        rankedMatches.begin(),
        rankedMatches.end(),
        [&searchTimer](
            const EpgSearchMatch& left,
            const EpgSearchMatch& right) {
            const int leftRank =
                fallbackRankForEvent(
                    searchTimer.query(),
                    left.event());
            const int rightRank =
                fallbackRankForEvent(
                    searchTimer.query(),
                    right.event());

            if (leftRank != rightRank)
            {
                return leftRank < rightRank;
            }

            if (left.event().startTime != right.event().startTime)
            {
                return left.event().startTime < right.event().startTime;
            }

            return left.event().title < right.event().title;
        });

    return EpgSearchResult(
        rankedMatches,
        result.totalCount(),
        result.limit(),
        result.offset());
}

void applySearchTimerOptions(
    EpgSearchRequest& request,
    const SearchTimer& searchTimer)
{
    if (hasExplicitComparisonOptions(searchTimer))
    {
        request.setSearchFields(
            searchTimer.comparisonOptions().compareTitle(),
            searchTimer.comparisonOptions().compareSubtitle(),
            searchTimer.comparisonOptions().compareSummary());
    }

    const EpgSearchMode searchMode =
        searchModeForSearchTimerMode(searchTimer.matchOptions().mode());

    request.setSearchMode(searchMode);

    if (searchMode == EpgSearchMode::Fuzzy)
    {
        request.setFuzzyTolerance(searchTimer.matchOptions().tolerance());
    }
}

EpgSearchRequest createPreviewRequest(
    const SearchTimer& searchTimer)
{
    EpgSearchRequest request =
        EpgSearchRequest::sorted(
            searchTimer.backendId(),
            searchTimer.query(),
            "",
            -1,
            0,
            0,
            0,
            EpgSearchSortField::StartTime,
            EpgSearchSortOrder::Ascending);

    applySearchTimerOptions(request, searchTimer);

    return request;
}

EpgSearchRequest createPreviewTypoFallbackRequest(
    const SearchTimer& searchTimer)
{
    EpgSearchRequest request = createPreviewRequest(searchTimer);
    request.setSearchMode(EpgSearchMode::Fuzzy);
    request.setFuzzyTolerance(previewTypoFallbackTolerance);
    return request;
}


int hhmmToMinutes(int value)
{
    const int positive = std::max(0, value);
    return (positive / 100) * 60 + (positive % 100);
}

bool validHhmm(int value)
{
    return value >= 0 && value <= 2359 && (value % 100) < 60;
}

int decimalAt(const std::string& value, std::size_t at, std::size_t count)
{
    if (at + count > value.size()) return -1;
    int number = 0;
    for (std::size_t i = at; i < at + count; ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(value[i]))) return -1;
        number = number * 10 + value[i] - '0';
    }
    return number;
}

bool validDate(int year, int month, int day)
{
    if (year < 1970 || year > 9999 || month < 1 || month > 12)
        return false;
    constexpr int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    const bool leap = (year % 4 == 0 && year % 100 != 0) ||
        year % 400 == 0;
    return day >= 1 && day <= days[month - 1] +
        (month == 2 && leap ? 1 : 0);
}

// VDR EPG uses epoch seconds; also accept ISO local and offset-aware times
// found in source/adapter fixtures. Resolve the event's local calendar day
// before applying epgsearch's time/weekday constraints.
bool eventLocalStart(const std::string& value, std::tm& out)
{
    if (value.size() >= 9U &&
        std::all_of(value.begin(), value.end(),
            [](unsigned char c) { return std::isdigit(c) != 0; }))
    {
        errno = 0;
        char* end = nullptr;
        const long long seconds = std::strtoll(value.c_str(), &end, 10);
        if (errno != 0 || !end || *end != '\0' || seconds < 0)
            return false;
        const std::time_t epoch = static_cast<std::time_t>(seconds);
        return static_cast<long long>(epoch) == seconds &&
            localtime_r(&epoch, &out) != nullptr;
    }

    if (value.size() != 16U && value.size() != 19U &&
        value.size() != 20U && value.size() != 25U)
        return false;
    if (value[4] != '-' || value[7] != '-' ||
        (value[10] != 'T' && value[10] != ' ') || value[13] != ':')
        return false;
    if (value.size() >= 19U && value[16] != ':')
        return false;

    const int year = decimalAt(value, 0U, 4U);
    const int month = decimalAt(value, 5U, 2U);
    const int day = decimalAt(value, 8U, 2U);
    const int hour = decimalAt(value, 11U, 2U);
    const int minute = decimalAt(value, 14U, 2U);
    const int second = value.size() == 16U
        ? 0 : decimalAt(value, 17U, 2U);
    if (!validDate(year, month, day) || hour < 0 || hour > 23 ||
        minute < 0 || minute > 59 || second < 0 || second > 59)
        return false;

    std::tm calendar{};
    calendar.tm_year = year - 1900;
    calendar.tm_mon = month - 1;
    calendar.tm_mday = day;
    calendar.tm_hour = hour;
    calendar.tm_min = minute;
    calendar.tm_sec = second;
    calendar.tm_isdst = -1;

    std::time_t epoch;
    if (value.size() == 16U || value.size() == 19U)
    {
        epoch = std::mktime(&calendar);
    }
    else
    {
        int offsetMinutes = 0;
        if (value.size() == 20U)
        {
            if (value[19] != 'Z') return false;
        }
        else
        {
            if ((value[19] != '+' && value[19] != '-') ||
                value[22] != ':') return false;
            const int hh = decimalAt(value, 20U, 2U);
            const int mm = decimalAt(value, 23U, 2U);
            if (hh < 0 || hh > 23 || mm < 0 || mm > 59)
                return false;
            offsetMinutes = (hh * 60 + mm) *
                (value[19] == '+' ? 1 : -1);
        }
        epoch = timegm(&calendar) -
            static_cast<std::time_t>(offsetMinutes) * 60;
    }
    return epoch != static_cast<std::time_t>(-1) &&
        localtime_r(&epoch, &out) != nullptr;
}

bool dayMatches(int dayOfWeek, int weekday)
{
    // epgsearch.conf: 0=Sunday .. 6=Saturday; a negative number is the
    // negated weekday bitmask (1=Sunday, 2=Monday, ..., 64=Saturday).
    return dayOfWeek >= 0
        ? weekday == dayOfWeek
        : ((-dayOfWeek) & (1 << weekday)) != 0;
}

// EPG/channel constraints apply before match/pagination: never filter a
// limited first page and then misreport the total or later candidates.
std::vector<VdrEvent> previewCandidates(
    const SearchTimer& timer,
    const std::vector<VdrEvent>& events,
    const std::vector<VdrChannel>& channels,
    std::vector<std::string>& warnings)
{
    const auto& filters = timer.filterOptions();
    const auto& options = timer.channelOptions();
    std::unordered_set<std::string> selectedChannels;
    bool restrictChannels = false;
    const int mode = filters.channelMode();

    if (mode == 1)
    {
        const std::string first = options.channelMinId();
        const std::string last = options.channelMaxId().empty()
            ? first : options.channelMaxId();
        if (!first.empty() && first == last)
        {
            selectedChannels.insert(first);
            restrictChannels = true;
        }
        else if (!first.empty() && !last.empty())
        {
            const auto findChannel = [&channels](const std::string& id) {
                return std::find_if(channels.begin(), channels.end(),
                    [&id](const VdrChannel& channel) { return channel.id == id; });
            };
            const auto lower = findChannel(first);
            const auto upper = findChannel(last);
            if (lower != channels.end() && upper != channels.end() &&
                lower->number > 0 && upper->number > 0)
            {
                const int lo = std::min(lower->number, upper->number);
                const int hi = std::max(lower->number, upper->number);
                for (const auto& channel : channels)
                    if (channel.number >= lo && channel.number <= hi)
                        selectedChannels.insert(channel.id);
                restrictChannels = true;
            }
        }
    }
    else if (mode == 2 && !channels.empty() && !options.channels().empty())
    {
        for (const auto& channel : channels)
            if (channel.group == options.channels())
                selectedChannels.insert(channel.id);
        restrictChannels = true;
    }
    else if (mode == 3 && !channels.empty())
    {
        for (const auto& channel : channels)
            if (!channel.encrypted)
                selectedChannels.insert(channel.id);
        restrictChannels = true;
    }

    if (mode != 0 && !restrictChannels)
        warnings.push_back(
            "Kanalfilter konnte ohne vollstaendige Kanalliste nicht ausgewertet werden.");
    const bool timeValid = validHhmm(filters.startTime()) &&
        validHhmm(filters.stopTime());
    const bool applyTime = filters.useTime() && timeValid;
    if (filters.useTime() && !timeValid)
        warnings.push_back(
            "Uhrzeitfilter hat ungueltige HHMM-Grenzen und wurde nicht angewendet.");

    const int weekday = filters.dayOfWeek();
    const bool dayValid = weekday >= -127 && weekday <= 6;
    const bool applyDay = filters.useDayOfWeek() && dayValid;
    if (filters.useDayOfWeek() && !dayValid)
        warnings.push_back(
            "Wochentagsfilter hat eine ungueltige epgsearch-Kodierung und wurde nicht angewendet.");

    const int startMinute = hhmmToMinutes(filters.startTime());
    const int stopMinute = hhmmToMinutes(filters.stopTime());
    bool unparseableStart = false;
    if (timer.repeatOptions().avoidRepeats())
        warnings.push_back(
            "Wiederholungspruefung ist nicht Bestandteil der EPG-Vorschau.");

    const int minDuration = hhmmToMinutes(filters.durationMinMinutes());
    const int maxDuration = hhmmToMinutes(filters.durationMaxMinutes());
    std::vector<VdrEvent> selected;
    selected.reserve(events.size());
    for (const auto& event : events)
    {
        if (restrictChannels && selectedChannels.count(event.channelId) == 0U)
            continue;
        if (filters.useDuration())
        {
            const int minutes = event.durationSeconds / 60;
            if (minutes < minDuration ||
                (filters.durationMaxMinutes() > 0 && minutes > maxDuration))
                continue;
        }
        if (applyTime || applyDay)
        {
            std::tm local{};
            if (!eventLocalStart(event.startTime, local))
            {
                // Do not silently turn uncertain EPG input into zero matches.
                unparseableStart = true;
            }
            else
            {
                const int eventMinute = local.tm_hour * 60 + local.tm_min;
                if (applyTime &&
                    (startMinute <= stopMinute
                        ? eventMinute < startMinute || eventMinute > stopMinute
                        : eventMinute < startMinute && eventMinute > stopMinute))
                    continue;
                if (applyDay && !dayMatches(weekday, local.tm_wday))
                    continue;
            }
        }
        selected.push_back(event);
    }
    if (unparseableStart)
        warnings.push_back(
            "Einige EPG-Startzeiten sind nicht lesbar; diese Treffer wurden ohne Zeitfilter belassen.");
    return selected;
}

EpgSearchResult paginateResult(
    const EpgSearchResult& result,
    int limit,
    int offset)
{
    const int safeOffset =
        offset < 0
            ? 0
            : offset;

    const std::vector<EpgSearchMatch>& sourceMatches =
        result.matches();

    std::vector<EpgSearchMatch> pagedMatches;

    if (safeOffset < static_cast<int>(sourceMatches.size()))
    {
        const int sourceSize =
            static_cast<int>(sourceMatches.size());

        const int end =
            limit > 0
                ? std::min(sourceSize, safeOffset + limit)
                : sourceSize;

        for (int index = safeOffset; index < end; ++index)
        {
            pagedMatches.push_back(sourceMatches.at(index));
        }
    }

    return EpgSearchResult(
        pagedMatches,
        result.totalCount(),
        limit,
        safeOffset);
}

} // namespace

SearchTimerPreviewResult SearchTimerPreviewService::preview(
    const SearchTimer& searchTimer,
    const std::vector<VdrEvent>& events,
    int limit,
    int offset,
    const std::vector<VdrChannel>& channels) const
{
    const SearchTimerPreviewEpgInputContextState epgInputContext =
        SearchTimerPreviewEpgInputContext::current();

    SearchTimerPreviewEpgInputContext::resetReady();

    EpgSearchService service;
    EpgSearchRequestMapper requestMapper;
    std::vector<std::string> warnings = epgInputContext.warnings;
    const std::vector<VdrEvent> candidates =
        previewCandidates(searchTimer, events, channels, warnings);

    const EpgSearchResult initialResult =
        service.search(
            candidates,
            requestMapper.map(createPreviewRequest(searchTimer)));

    const EpgSearchResult unpagedResult =
        shouldApplyPreviewTypoFallback(searchTimer, initialResult)
            ? rankTypoFallbackResult(
                  searchTimer,
                  service.search(
                      candidates,
                      requestMapper.map(
                          createPreviewTypoFallbackRequest(searchTimer))))
            : initialResult;

    return SearchTimerPreviewResult(
        searchTimer,
        paginateResult(
            unpagedResult,
            limit,
            offset),
        epgInputContext.status,
        epgInputContext.available,
        warnings);
}
