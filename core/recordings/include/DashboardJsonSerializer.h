#pragma once

#include "DashboardOverview.h"

#include <string>

class DashboardJsonSerializer
{
public:
    std::string serialize(
        const DashboardOverview& overview) const;
};
