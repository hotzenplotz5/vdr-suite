#pragma once

#include "EpgSearchRequest.h"
#include "VdrEvent.h"

class EpgSearchMatcher
{
public:
    bool matches(
        const VdrEvent& event,
        const EpgSearchRequest& request) const;
};
