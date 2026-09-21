#pragma once

#include "ISearchTimerCommandExecutor.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerDeleteResult.h"

class SearchTimerDeleteService
{
public:
    SearchTimerDeleteResult remove(
        const SearchTimerDeleteRequest& request,
        ISearchTimerCommandExecutor& executor) const;
};
