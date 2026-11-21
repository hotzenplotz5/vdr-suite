#pragma once

#include "EpgSearchResult.h"
#include "SearchTimer.h"

#include <string>
#include <utility>
#include <vector>

class SearchTimerPreviewResult {
public:
    SearchTimerPreviewResult(
        SearchTimer searchTimer,
        EpgSearchResult searchResult)
        : SearchTimerPreviewResult(
              std::move(searchTimer),
              std::move(searchResult),
              "ready",
              true,
              {})
    {
    }

    SearchTimerPreviewResult(
        SearchTimer searchTimer,
        EpgSearchResult searchResult,
        std::string epgInputStatus,
        bool epgInputAvailable,
        std::vector<std::string> warnings)
        : SearchTimerPreviewResult(
              std::move(searchTimer),
              std::move(searchResult),
              std::move(epgInputStatus),
              epgInputAvailable,
              std::move(warnings),
              "suite-epg-cache")
    {
    }

    SearchTimerPreviewResult(
        SearchTimer searchTimer,
        EpgSearchResult searchResult,
        std::string epgInputStatus,
        bool epgInputAvailable,
        std::vector<std::string> warnings,
        std::string previewEngine)
        : searchTimer_(std::move(searchTimer)),
          searchResult_(std::move(searchResult)),
          epgInputStatus_(std::move(epgInputStatus)),
          epgInputAvailable_(epgInputAvailable),
          warnings_(std::move(warnings)),
          previewEngine_(std::move(previewEngine))
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

    const std::string& epgInputStatus() const
    {
        return epgInputStatus_;
    }

    bool epgInputAvailable() const
    {
        return epgInputAvailable_;
    }

    const std::vector<std::string>& warnings() const
    {
        return warnings_;
    }

    const std::string& previewEngine() const
    {
        return previewEngine_;
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
    std::string epgInputStatus_;
    bool epgInputAvailable_ = true;
    std::vector<std::string> warnings_;
    std::string previewEngine_ = "suite-epg-cache";
};
