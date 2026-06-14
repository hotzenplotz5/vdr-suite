#pragma once

#include "VdrRecording.h"

#include <utility>
#include <vector>

class VdrRecordingQueryResult
{
public:
    VdrRecordingQueryResult(
        std::vector<VdrRecording> recordings,
        int totalCount,
        int limit,
        int offset)
        : recordings_(std::move(recordings)),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    static VdrRecordingQueryResult empty(
        int limit,
        int offset)
    {
        return VdrRecordingQueryResult(
            {},
            0,
            limit,
            offset);
    }

    const std::vector<VdrRecording>& recordings() const
    {
        return recordings_;
    }

    int totalCount() const
    {
        return totalCount_;
    }

    int returnedCount() const
    {
        return static_cast<int>(recordings_.size());
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
        return recordings_.empty();
    }

private:
    std::vector<VdrRecording> recordings_;
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};
