#pragma once

#include "EpgSearchQuery.h"
#include "EpgSearchRequest.h"

class EpgSearchRequestMapper {
public:
    EpgSearchQuery map(
        const EpgSearchRequest& request) const;
};
