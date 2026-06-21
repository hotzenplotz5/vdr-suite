#pragma once

#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"

class ISearchTimerCommandExecutor
{
public:
    virtual ~ISearchTimerCommandExecutor() = default;

    virtual SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) = 0;
};
