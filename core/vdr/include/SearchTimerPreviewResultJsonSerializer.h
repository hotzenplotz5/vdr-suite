#pragma once

#include "SearchTimerPreviewResult.h"

#include <string>

class SearchTimerPreviewResultJsonSerializer {
public:
    std::string serialize(
        const SearchTimerPreviewResult& result) const;
};
