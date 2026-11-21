#pragma once

#include "Person.h"
#include "VdrRecording.h"

#include <utility>
#include <vector>

class RecordingPersonSearchMatch
{
public:
    RecordingPersonSearchMatch(
        VdrRecording recording,
        Person person)
        : recording_(std::move(recording)),
          person_(std::move(person))
    {
    }

    const VdrRecording& recording() const
    {
        return recording_;
    }

    const Person& person() const
    {
        return person_;
    }

private:
    VdrRecording recording_;
    Person person_;
};

class RecordingPersonSearchResult
{
public:
    static RecordingPersonSearchResult empty(
        int limit,
        int offset)
    {
        return RecordingPersonSearchResult(
            {},
            0,
            limit,
            offset);
    }

    static RecordingPersonSearchResult from(
        std::vector<RecordingPersonSearchMatch> matches,
        int totalCount,
        int limit,
        int offset)
    {
        return RecordingPersonSearchResult(
            std::move(matches),
            totalCount,
            limit,
            offset);
    }

    const std::vector<RecordingPersonSearchMatch>& matches() const
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
    RecordingPersonSearchResult(
        std::vector<RecordingPersonSearchMatch> matches,
        int totalCount,
        int limit,
        int offset)
        : matches_(std::move(matches)),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    std::vector<RecordingPersonSearchMatch> matches_;
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};
