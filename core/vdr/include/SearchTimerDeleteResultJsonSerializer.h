#pragma once

#include "SearchTimerDeleteResult.h"

#include <string>

class SearchTimerDeleteResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerDeleteResult& result) const;
};
