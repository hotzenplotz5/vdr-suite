#pragma once

#include "Person.h"

#include <vector>

class PersonQueryResult {
public:
    static PersonQueryResult empty(
        int limit,
        int offset)
    {
        return PersonQueryResult(
            PersonCollection::createEmpty(),
            0,
            limit,
            offset);
    }

    static PersonQueryResult from(
        const PersonCollection& persons,
        int totalCount,
        int limit,
        int offset)
    {
        return PersonQueryResult(
            persons,
            totalCount,
            limit,
            offset);
    }

    const PersonCollection& persons() const
    {
        return persons_;
    }

    int totalCount() const
    {
        return totalCount_;
    }

    int returnedCount() const
    {
        return persons_.size();
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
    PersonQueryResult(
        const PersonCollection& persons,
        int totalCount,
        int limit,
        int offset)
        : persons_(persons),
          totalCount_(totalCount),
          limit_(limit),
          offset_(offset)
    {
    }

    PersonCollection persons_ = PersonCollection::createEmpty();
    int totalCount_ = 0;
    int limit_ = 0;
    int offset_ = 0;
};
