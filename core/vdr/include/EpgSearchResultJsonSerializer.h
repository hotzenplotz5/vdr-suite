#pragma once

#include "EpgSearchResult.h"

#include <string>

class EpgSearchResultJsonSerializer
{
public:
    std::string serialize(
        const EpgSearchResult& result) const;
};
