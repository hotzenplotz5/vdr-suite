#include "MockVdrAdapter.h"
#include "PollingService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>

static void test_polling_service_updates_snapshot()
{
    MockVdrAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    PollingService pollingService(builder);

    pollingService.poll();

    const auto& snapshot = pollingService.snapshot();

    assert(snapshot.status.enabled == true);
    assert(snapshot.recordings.size() == 2);
    assert(snapshot.timers.size() == 1);
    assert(snapshot.channels.size() == 3);
    assert(snapshot.events.size() == 2);
}

int main()
{
    test_polling_service_updates_snapshot();

    std::cout << "test_polling_service passed" << std::endl;
    return 0;
}
