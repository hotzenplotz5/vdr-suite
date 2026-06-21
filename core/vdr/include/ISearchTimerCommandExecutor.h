#pragma once

#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerUpdateRequest.h"
#include "SearchTimerUpdateResult.h"

class ISearchTimerCommandExecutor
{
public:
    virtual ~ISearchTimerCommandExecutor() = default;

    virtual SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request) = 0;

    virtual SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request) = 0;
};
