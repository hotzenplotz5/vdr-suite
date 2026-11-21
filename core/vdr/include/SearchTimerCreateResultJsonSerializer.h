#pragma once

#include "SearchTimerCreateResult.h"

#include <string>

class SearchTimerCreateResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerCreateResult& result) const;
};
