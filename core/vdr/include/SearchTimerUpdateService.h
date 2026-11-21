#pragma once

#include "ISearchTimerCommandExecutor.h"
#include "SearchTimerUpdateRequest.h"
#include "SearchTimerUpdateResult.h"

class SearchTimerUpdateService
{
public:
    SearchTimerUpdateResult update(
        const SearchTimerUpdateRequest& request,
        ISearchTimerCommandExecutor& executor) const;
};
