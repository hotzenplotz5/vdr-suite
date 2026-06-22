#pragma once

#include "EpgSearchQuery.h"
#include "EpgSearchResult.h"
#include "VdrEvent.h"

#include <vector>

class EpgSearchService {
public:
    EpgSearchResult search(
        const std::vector<VdrEvent>& events,
        const EpgSearchQuery& query) const;
};
