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
    int offset) const
{
    const SearchTimerPreviewEpgInputContextState epgInputContext =
        SearchTimerPreviewEpgInputContext::current();

    SearchTimerPreviewEpgInputContext::resetReady();

    EpgSearchService service;
    EpgSearchRequestMapper requestMapper;

    const EpgSearchResult initialResult =
        service.search(
            events,
            requestMapper.map(createPreviewRequest(searchTimer)));

    const EpgSearchResult unpagedResult =
        shouldApplyPreviewTypoFallback(searchTimer, initialResult)
            ? rankTypoFallbackResult(
                  searchTimer,
                  service.search(
                      events,
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
        epgInputContext.warnings);
}
