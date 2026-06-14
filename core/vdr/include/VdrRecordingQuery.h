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

    static VdrRecordingQuery sorted(
        const std::string& titleFilter,
        const std::string& pathFilter,
        int limit,
        int offset,
        VdrRecordingSortField sortField,
        VdrRecordingSortOrder sortOrder)
    {
        VdrRecordingQuery query =
            filtered(
                titleFilter,
                pathFilter,
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

    bool hasTitleFilter() const
    {
        return !titleFilter_.empty();
    }

    bool hasPathFilter() const
    {
        return !pathFilter_.empty();
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

private:
    std::string titleFilter_;
    std::string pathFilter_;
    int limit_ = 0;
    int offset_ = 0;
    VdrRecordingSortField sortField_ = VdrRecordingSortField::None;
    VdrRecordingSortOrder sortOrder_ = VdrRecordingSortOrder::Ascending;
};
