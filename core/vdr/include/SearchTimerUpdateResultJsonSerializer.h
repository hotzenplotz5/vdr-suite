#pragma once

#include "SearchTimerUpdateResult.h"

#include <string>

class SearchTimerUpdateResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerUpdateResult& result) const;
};
