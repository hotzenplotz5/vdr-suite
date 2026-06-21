#pragma once

#include "ISearchTimerCommandExecutor.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateResult.h"

class SearchTimerCreateService
{
public:
    SearchTimerCreateResult create(
        const SearchTimerCreateRequest& request,
        ISearchTimerCommandExecutor& executor) const;
};
