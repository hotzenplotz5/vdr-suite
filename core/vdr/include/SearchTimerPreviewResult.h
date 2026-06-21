#pragma once

#include "EpgSearchResult.h"
#include "SearchTimer.h"

class SearchTimerPreviewResult {
public:
    SearchTimerPreviewResult(
        SearchTimer searchTimer,
        EpgSearchResult searchResult)
        : searchTimer_(std::move(searchTimer)),
          searchResult_(std::move(searchResult))
    {
    }

    const SearchTimer& searchTimer() const
    {
        return searchTimer_;
    }

    const EpgSearchResult& searchResult() const
    {
        return searchResult_;
    }

    int totalCount() const
    {
        return searchResult_.totalCount();
    }

    int returnedCount() const
    {
        return searchResult_.returnedCount();
    }

    bool empty() const
    {
        return searchResult_.empty();
    }

private:
    SearchTimer searchTimer_;
    EpgSearchResult searchResult_;
};
