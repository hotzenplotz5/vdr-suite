#pragma once

#include "VdrOverview.h"

class ISnapshotAccessService;
class VdrService;

class VdrOverviewService
{
public:
    explicit VdrOverviewService(VdrService& vdrService);
    explicit VdrOverviewService(ISnapshotAccessService& snapshotAccessService);

    VdrOverview getOverview() const;

private:
    VdrOverview getLiveOverview() const;
    VdrOverview getSnapshotOverview() const;

    VdrService* vdrService_ = nullptr;
    ISnapshotAccessService* snapshotAccessService_ = nullptr;
};
