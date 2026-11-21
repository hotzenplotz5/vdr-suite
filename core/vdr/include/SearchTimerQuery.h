#pragma once

#include "SearchTimer.h"

#include <string>

class SearchTimerQuery {
public:
    static SearchTimerQuery all()
    {
        return SearchTimerQuery();
    }

    static SearchTimerQuery limited(
        int limit,
        int offset)
    {
        SearchTimerQuery query;
        query.limit_ = limit;
        query.offset_ = offset;
        return query;
    }

    static SearchTimerQuery byBackend(
        const std::string& backendId)
    {
        SearchTimerQuery query;
        query.backendId_ = backendId;
        return query;
    }

    static SearchTimerQuery byState(
        SearchTimerState state)
    {
        SearchTimerQuery query;
        query.state_ = state;
        query.hasState_ = true;
        return query;
    }

    static SearchTimerQuery byText(
        const std::string& text)
    {
        SearchTimerQuery query;
        query.text_ = text;
        return query;
    }

    SearchTimerQuery& withBackend(
        const std::string& backendId)
    {
        backendId_ = backendId;
        return *this;
    }

    SearchTimerQuery& withState(
        SearchTimerState state)
    {
        state_ = state;
        hasState_ = true;
        return *this;
    }

    SearchTimerQuery& withText(
        const std::string& text)
    {
        text_ = text;
        return *this;
    }

    SearchTimerQuery& withLimit(
        int limit,
        int offset)
    {
        limit_ = limit;
        offset_ = offset;
        return *this;
    }

    bool isEmpty() const
    {
        return !hasBackend()
            && !hasState()
            && !hasText()
            && !hasLimit();
    }

    bool matchesAll() const
    {
        return isEmpty();
    }

    bool hasBackend() const
    {
        return !backendId_.empty();
    }

    const std::string& backendId() const
    {
        return backendId_;
    }

    bool hasState() const
    {
        return hasState_;
    }

    SearchTimerState state() const
    {
        return state_;
    }

    bool hasText() const
    {
        return !text_.empty();
    }

    const std::string& text() const
    {
        return text_;
    }

    bool hasLimit() const
    {
        return limit_ > 0;
    }

    int limit() const
    {
        return limit_;
    }

    int offset() const
    {
        return offset_;
    }

private:
    std::string backendId_;
    SearchTimerState state_ = SearchTimerState::Unknown;
    bool hasState_ = false;
    std::string text_;
    int limit_ = 0;
    int offset_ = 0;
};