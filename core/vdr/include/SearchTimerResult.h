#pragma once

#include "SearchTimer.h"

#include <utility>
#include <vector>

class SearchTimerResult {
public:
    static SearchTimerResult empty(
        int limit,
        int offset)
    {
        return SearchTimerResult(
            {},
            0,
            limit,
            offset);
    }

    static SearchTimerResult from(
        std::vector<SearchTimer> items,
        int totalCount,
        int limit,
        int offset)
    {
        return SearchTimerResult(
            std::move(items),
            totalCount,
            limit,
            offset);
    }

    const std::vector<SearchTimer>& items() const
    {
        return items_;
    }

    int totalCount() const
    {
        return totalCount_;
    }

    int returnedCount() const
    {
        return static_cast<int>(items_.size());
    }

    int limit() const
    {
        return limit_;
    }

    int offset() const
    {
        return offset_;
    }

    bool empty() const
    {
        return items_.empty();
    }

private:
    SearchTimerResult(
        std::vector<SearchTimer> items,
        int totalCount,
        int limit,
        int offset)
        : items_(std::move(items)),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    std::vector<SearchTimer> items_;
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};