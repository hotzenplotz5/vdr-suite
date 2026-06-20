#pragma once

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

class ISearchTimerDataSource {
public:
    virtual ~ISearchTimerDataSource() = default;

    virtual SearchTimerResult list(
        const SearchTimerQuery& query) const = 0;
};
