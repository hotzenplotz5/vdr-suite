#pragma once

#include <string>

enum class VdrRecordingSortField
{
    None,
    Title,
    StartTime,
    Duration,
    Size
};

enum class VdrRecordingSortOrder
{
    Ascending,
    Descending
};

class VdrRecordingQuery
{
public:
    static VdrRecordingQuery all()
    {
        return VdrRecordingQuery();
    }

    static VdrRecordingQuery limited(
        int limit,
        int offset)
    {
        VdrRecordingQuery query;
        query.limit_ = limit;
        query.offset_ = offset;
        return query;
    }

    static VdrRecordingQuery byTitle(
        const std::string& titleFilter,
        int limit,
        int offset)
    {
        VdrRecordingQuery query;
        query.titleFilter_ = titleFilter;
        query.limit_ = limit;
        query.offset_ = offset;
        return query;
    }

    static VdrRecordingQuery filtered(
        const std::string& titleFilter,
        const std::string& pathFilter,
        int limit,
        int offset)
    {
        VdrRecordingQuery query;
        query.titleFilter_ = titleFilter;
        query.pathFilter_ = pathFilter;
        query.limit_ = limit;
        query.offset_ = offset;
        return query;
    }

    static VdrRecordingQuery ranged(
        const std::string& titleFilter,
        const std::string& pathFilter,
        const std::string& fromStartTime,
        const std::string& toStartTime,
        int limit,
        int offset)
    {
        VdrRecordingQuery query =
            filtered(
                titleFilter,
                pathFilter,
                limit,
                offset);

        query.fromStartTime_ = fromStartTime;
        query.toStartTime_ = toStartTime;

        return query;
    }

    static VdrRecordingQuery durationRanged(
        const std::string& titleFilter,
        const std::string& pathFilter,
        const std::string& fromStartTime,
        const std::string& toStartTime,
        int minDurationSeconds,
        int maxDurationSeconds,
        int limit,
        int offset)
    {
        VdrRecordingQuery query =
            ranged(
                titleFilter,
                pathFilter,
                fromStartTime,
                toStartTime,
                limit,
                offset);

        query.minDurationSeconds_ = minDurationSeconds;
        query.maxDurationSeconds_ = maxDurationSeconds;

        return query;
    }

    static VdrRecordingQuery sorted(
        const std::string& titleFilter,
        const std::string& pathFilter,
        int limit,
        int offset,
        VdrRecordingSortField sortField,
        VdrRecordingSortOrder sortOrder)
    {
        VdrRecordingQuery query =
            ranged(
                titleFilter,
                pathFilter,
                "",
                "",
                limit,
                offset);

        query.sortField_ = sortField;
        query.sortOrder_ = sortOrder;

        return query;
    }

    static VdrRecordingQuery sortedRanged(
        const std::string& titleFilter,
        const std::string& pathFilter,
        const std::string& fromStartTime,
        const std::string& toStartTime,
        int limit,
        int offset,
        VdrRecordingSortField sortField,
        VdrRecordingSortOrder sortOrder)
    {
        VdrRecordingQuery query =
            durationRanged(
                titleFilter,
                pathFilter,
                fromStartTime,
                toStartTime,
                0,
                0,
                limit,
                offset);

        query.sortField_ = sortField;
        query.sortOrder_ = sortOrder;

        return query;
    }

    static VdrRecordingQuery sortedDurationRanged(
        const std::string& titleFilter,
        const std::string& pathFilter,
        const std::string& fromStartTime,
        const std::string& toStartTime,
        int minDurationSeconds,
        int maxDurationSeconds,
        int limit,
        int offset,
        VdrRecordingSortField sortField,
        VdrRecordingSortOrder sortOrder)
    {
        VdrRecordingQuery query =
            durationRanged(
                titleFilter,
                pathFilter,
                fromStartTime,
                toStartTime,
                minDurationSeconds,
                maxDurationSeconds,
                limit,
                offset);

        query.sortField_ = sortField;
        query.sortOrder_ = sortOrder;

        return query;
    }

    const std::string& titleFilter() const
    {
        return titleFilter_;
    }

    const std::string& pathFilter() const
    {
        return pathFilter_;
    }

    const std::string& backendFilter() const
    {
        return backendFilter_;
    }

    bool hasTitleFilter() const
    {
        return !titleFilter_.empty();
    }

    bool hasPathFilter() const
    {
        return !pathFilter_.empty();
    }

    bool hasBackendFilter() const
    {
        return !backendFilter_.empty();
    }

    const std::string& fromStartTime() const
    {
        return fromStartTime_;
    }

    const std::string& toStartTime() const
    {
        return toStartTime_;
    }

    bool hasFromStartTime() const
    {
        return !fromStartTime_.empty();
    }

    bool hasToStartTime() const
    {
        return !toStartTime_.empty();
    }

    int minDurationSeconds() const
    {
        return minDurationSeconds_;
    }

    int maxDurationSeconds() const
    {
        return maxDurationSeconds_;
    }

    bool hasMinDurationSeconds() const
    {
        return minDurationSeconds_ > 0;
    }

    bool hasMaxDurationSeconds() const
    {
        return maxDurationSeconds_ > 0;
    }

    int limit() const
    {
        return limit_;
    }

    int offset() const
    {
        return offset_;
    }

    bool hasLimit() const
    {
        return limit_ > 0;
    }

    VdrRecordingSortField sortField() const
    {
        return sortField_;
    }

    VdrRecordingSortOrder sortOrder() const
    {
        return sortOrder_;
    }

    bool hasSort() const
    {
        return sortField_ != VdrRecordingSortField::None;
    }

    bool sortDescending() const
    {
        return sortOrder_ == VdrRecordingSortOrder::Descending;
    }

    void setBackendFilter(
        const std::string& backendFilter)
    {
        backendFilter_ = backendFilter;
    }

private:
    std::string titleFilter_;
    std::string pathFilter_;
    std::string backendFilter_;
    std::string fromStartTime_;
    std::string toStartTime_;
    int minDurationSeconds_ = 0;
    int maxDurationSeconds_ = 0;
    int limit_ = 0;
    int offset_ = 0;
    VdrRecordingSortField sortField_ = VdrRecordingSortField::None;
    VdrRecordingSortOrder sortOrder_ = VdrRecordingSortOrder::Ascending;
};
