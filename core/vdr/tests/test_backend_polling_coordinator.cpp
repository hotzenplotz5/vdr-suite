#include "BackendPollingCoordinator.h"
#include "IVdrAdapter.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <vector>

class CountingVdrAdapter : public IVdrAdapter
{
public:
    mutable int statusReadCount = 0;

    VdrStatus getStatus() const override
    {
        ++statusReadCount;

        VdrStatus status;
        status.enabled = true;
        status.mode = "test";
        status.state = "connected";
        return status;
    }

    std::vector<VdrEvent> getEvents() const override
    {
        return {};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        return {};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        return {};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {};
    }

    VdrChangeState getChangeState() const override
    {
        return {};
    }
};

static void test_coordinator_tracks_backend_count()
{
    BackendPollingCoordinator coordinator;

    CountingVdrAdapter firstAdapter;
    CountingVdrAdapter secondAdapter;

    VdrService firstService(firstAdapter);
    VdrService secondService(secondAdapter);

    VdrSnapshotBuilder firstBuilder(
        firstService,
        "home-vdr",
        nullptr,
        nullptr);

    VdrSnapshotBuilder secondBuilder(
        secondService,
        "parents-vdr",
        nullptr,
        nullptr);

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);

    PollingService firstPollingService(
        firstBuilder,
        firstService,
        cacheService,
        "home-vdr");

    PollingService secondPollingService(
        secondBuilder,
        secondService,
        cacheService,
        "parents-vdr");

    coordinator.addPollingService("home-vdr", firstPollingService);
    coordinator.addPollingService("parents-vdr", secondPollingService);

    assert(coordinator.backendCount() == 2);
}

static void test_coordinator_polls_all_backends()
{
    BackendPollingCoordinator coordinator;

    CountingVdrAdapter firstAdapter;
    CountingVdrAdapter secondAdapter;

    VdrService firstService(firstAdapter);
    VdrService secondService(secondAdapter);

    VdrSnapshotBuilder firstBuilder(
        firstService,
        "home-vdr",
        nullptr,
        nullptr);

    VdrSnapshotBuilder secondBuilder(
        secondService,
        "parents-vdr",
        nullptr,
        nullptr);

    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);

    PollingService firstPollingService(
        firstBuilder,
        firstService,
        cacheService,
        "home-vdr");

    PollingService secondPollingService(
        secondBuilder,
        secondService,
        cacheService,
        "parents-vdr");

    coordinator.addPollingService("home-vdr", firstPollingService);
    coordinator.addPollingService("parents-vdr", secondPollingService);

    coordinator.pollAll();

    assert(firstAdapter.statusReadCount == 1);
    assert(secondAdapter.statusReadCount == 1);

    assert(cache.hasSnapshotForBackend("home-vdr"));
    assert(cache.hasSnapshotForBackend("parents-vdr"));
}

int main()
{
    test_coordinator_tracks_backend_count();
    test_coordinator_polls_all_backends();

    std::cout << "test_backend_polling_coordinator passed" << std::endl;
    return 0;
}
