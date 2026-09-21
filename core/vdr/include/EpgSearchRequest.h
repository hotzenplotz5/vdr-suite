#pragma once

#include "EpgSearchQuery.h"

#include <string>

enum class EpgSearchSortField
{
    None,
    StartTime,
    Title,
    Duration
};

enum class EpgSearchSortOrder
{
    Ascending,
    Descending
};

class EpgSearchRequest
{
public:
    static EpgSearchRequest all()
    {
        return EpgSearchRequest();
    }

    static EpgSearchRequest text(
        const std::string& queryText,
        int limit,
        int offset)
    {
        EpgSearchRequest request;
        request.queryText_ = queryText;
        request.limit_ = limit;
        request.offset_ = offset;
        return request;
    }

    static EpgSearchRequest window(
        const std::string& queryText,
        const std::string& channelId,
        int from,
        int timespan,
        int limit,
        int offset)
    {
        EpgSearchRequest request =
            text(
                queryText,
                limit,
                offset);

        request.channelId_ = channelId;
        request.from_ = from;
        request.timespan_ = timespan;

        return request;
    }

    static EpgSearchRequest backendWindow(
        const std::string& backendId,
        const std::string& queryText,
        const std::string& channelId,
        int from,
        int timespan,
        int limit,
        int offset)
    {
        EpgSearchRequest request =
            window(
                queryText,
                channelId,
                from,
                timespan,
                limit,
                offset);

        request.backendId_ = backendId;

        return request;
    }

    static EpgSearchRequest sorted(
        const std::string& backendId,
        const std::string& queryText,
        const std::string& channelId,
        int from,
        int timespan,
        int limit,
        int offset,
        EpgSearchSortField sortField,
        EpgSearchSortOrder sortOrder)
    {
        EpgSearchRequest request =
            backendWindow(
                backendId,
                queryText,
                channelId,
                from,
                timespan,
                limit,
                offset);

        request.sortField_ = sortField;
        request.sortOrder_ = sortOrder;

        return request;
    }

    const std::string& queryText() const
    {
        return queryText_;
    }

    bool hasQueryText() const
    {
        return !queryText_.empty();
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasBackendId() const
    {
        return !backendId_.empty();
    }

    const std::string& channelId() const
    {
        return channelId_;
    }

    bool hasChannelId() const
    {
        return !channelId_.empty();
    }

    int from() const
    {
        return from_;
    }

    bool hasFrom() const
    {
        return from_ >= 0;
    }

    int timespan() const
    {
        return timespan_;
    }

    bool hasTimespan() const
    {
        return timespan_ > 0;
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

    bool searchTitle() const
    {
        return searchTitle_;
    }

    bool searchSubtitle() const
    {
        return searchSubtitle_;
    }

    bool searchDescription() const
    {
        return searchDescription_;
    }

    void setSearchFields(
        bool title,
        bool subtitle,
        bool description)
    {
        searchTitle_ = title;
        searchSubtitle_ = subtitle;
        searchDescription_ = description;
    }

    void setSearchMode(
        EpgSearchMode mode)
    {
        searchMode_ = mode;
        hasSearchMode_ = true;
    }

    bool hasSearchMode() const
    {
        return hasSearchMode_;
    }

    EpgSearchMode searchMode() const
    {
        return searchMode_;
    }

    void setFuzzyTolerance(
        int tolerance)
    {
        fuzzyTolerance_ = tolerance;
        hasFuzzyTolerance_ = true;
    }

    bool hasFuzzyTolerance() const
    {
        return hasFuzzyTolerance_;
    }

    int fuzzyTolerance() const
    {
        return fuzzyTolerance_;
    }

    bool hasSearchField() const
    {
        return searchTitle_ || searchSubtitle_ || searchDescription_;
    }

    EpgSearchSortField sortField() const
    {
        return sortField_;
    }

    EpgSearchSortOrder sortOrder() const
    {
        return sortOrder_;
    }

    bool hasSort() const
    {
        return sortField_ != EpgSearchSortField::None;
    }

    bool sortDescending() const
    {
        return sortOrder_ == EpgSearchSortOrder::Descending;
    }

private:
    std::string backendId_;
    std::string queryText_;
    std::string channelId_;

    int from_ = -1;
    int timespan_ = 0;
    int limit_ = 0;
    int offset_ = 0;

    bool searchTitle_ = true;
    bool searchSubtitle_ = true;
    bool searchDescription_ = true;

    EpgSearchMode searchMode_ = EpgSearchMode::Phrase;
    bool hasSearchMode_ = false;

    int fuzzyTolerance_ = 0;
    bool hasFuzzyTolerance_ = false;

    EpgSearchSortField sortField_ = EpgSearchSortField::None;
    EpgSearchSortOrder sortOrder_ = EpgSearchSortOrder::Ascending;
};
