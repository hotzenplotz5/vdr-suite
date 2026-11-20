#include "SearchTimerPreviewService.h"

#include "EpgSearchMatch.h"
#include "EpgSearchRequest.h"
#include "EpgSearchRequestMapper.h"
#include "EpgSearchResult.h"
#include "EpgSearchService.h"
#include "SearchTimerPreviewEpgInputContext.h"

#include <algorithm>
#include <cctype>
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
    if (filters.useTime() || filters.useDayOfWeek())
        warnings.push_back(
            "Uhrzeit- und Wochentagsfilter werden in dieser EPG-Vorschau noch nicht ausgewertet.");
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
        selected.push_back(event);
    }
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
