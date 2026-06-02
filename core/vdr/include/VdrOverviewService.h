#pragma once

#include "VdrOverview.h"

class VdrService;

class VdrOverviewService
{
public:
    explicit VdrOverviewService(VdrService& vdrService);

    VdrOverview getOverview() const;

private:
    VdrService& vdrService_;
};
