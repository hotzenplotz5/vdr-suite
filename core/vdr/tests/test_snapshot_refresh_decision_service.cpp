#include "SnapshotRefreshDecisionService.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    SnapshotRefreshDecisionService service;

    auto noRefresh = service.decide({});
    assert(noRefresh.shouldRefreshSnapshot() == false);

    std::vector<VdrChangeEvent> events;
    events.emplace_back(VdrChangeType::ChannelsChanged);

    auto refresh = service.decide(events);
    assert(refresh.shouldRefreshSnapshot() == true);

    std::cout << "test_snapshot_refresh_decision_service passed" << std::endl;
    return 0;
}
