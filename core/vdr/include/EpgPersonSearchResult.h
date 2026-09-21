#pragma once

#include "Person.h"
#include "VdrEvent.h"

#include <string>
#include <utility>
#include <vector>

class EpgPersonSearchMatch
{
public:
    EpgPersonSearchMatch(
        VdrEvent event,
        std::string backendId,
        Person person)
        : event_(std::move(event)),
          backendId_(std::move(backendId)),
          person_(std::move(person))
    {
    }

    const VdrEvent& event() const
    {
        return event_;
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasBackendId() const
    {
        return !backendId_.empty();
    }

    const Person& person() const
    {
        return person_;
    }

private:
    VdrEvent event_;
    std::string backendId_;
    Person person_;
};

class EpgPersonSearchResult
{
public:
    static EpgPersonSearchResult empty(
        int limit,
        int offset)
    {
        return EpgPersonSearchResult(
            {},
            0,
            limit,
            offset);
    }

    static EpgPersonSearchResult from(
        std::vector<EpgPersonSearchMatch> matches,
        int totalCount,
        int limit,
        int offset)
    {
        return EpgPersonSearchResult(
            std::move(matches),
            totalCount,
            limit,
            offset);
    }

    const std::vector<EpgPersonSearchMatch>& matches() const
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
    EpgPersonSearchResult(
        std::vector<EpgPersonSearchMatch> matches,
        int totalCount,
        int limit,
        int offset)
        : matches_(std::move(matches)),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    std::vector<EpgPersonSearchMatch> matches_;
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};
