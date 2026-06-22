#pragma once

#include "EpgSearchQuery.h"
#include "VdrEvent.h"

class EpgSearchMatcher {
public:
    bool matches(
        const VdrEvent& event,
        const EpgSearchQuery& query) const;
};
