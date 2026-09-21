#pragma once

#include "SearchTimer.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <vector>

class SearchTimerService {
public:
    SearchTimerResult list(
        const std::vector<SearchTimer>& timers,
        const SearchTimerQuery& query) const;
};