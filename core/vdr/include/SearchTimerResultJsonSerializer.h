#pragma once

#include "SearchTimerResult.h"

#include <string>

class SearchTimerResultJsonSerializer
{
public:
    std::string serialize(
        const SearchTimerResult& result) const;
    std::string serializePublicSettings(const SearchTimer& timer) const;
};