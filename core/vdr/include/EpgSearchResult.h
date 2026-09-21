#pragma once

#include "EpgSearchMatch.h"

#include <utility>
#include <vector>

class EpgSearchResult
{
public:
    EpgSearchResult(
        std::vector<EpgSearchMatch> matches,
        int totalCount,
        int limit,
        int offset)
        : matches_(std::move(matches)),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    static EpgSearchResult empty(
        int limit,
        int offset)
    {
        return EpgSearchResult(
            {},
            0,
            limit,
            offset);
    }

    const std::vector<EpgSearchMatch>& matches() const
    {
        return matches_;
    }

    int totalCount() const
    {
        return totalCount_;
    }

    int returnedCount() const
    {
        return static_cast<int>(matches_.size());
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
        return matches_.empty();
    }

private:
    std::vector<EpgSearchMatch> matches_;
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};
