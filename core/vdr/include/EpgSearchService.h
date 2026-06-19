#pragma once

#include "EpgSearchRequest.h"
#include "EpgSearchResult.h"
#include "VdrEvent.h"

#include <vector>

class EpgSearchService
{
public:
    EpgSearchResult search(
        const std::vector<VdrEvent>& events,
        const EpgSearchRequest& request) const;
};
