#pragma once

#include <string>

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

private:
    std::string titleFilter_;
    std::string pathFilter_;
    int limit_ = 0;
    int offset_ = 0;
};
