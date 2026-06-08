#pragma once

#include "VdrOverview.h"

#include <string>

class VdrOverviewJsonSerializer
{
public:
    std::string serialize(
        const VdrOverview& overview) const;
};
