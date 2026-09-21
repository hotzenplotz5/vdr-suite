#include "SnapshotCache.h"

#include <cassert>

static void test_single_snapshot_compatibility()
{
    SnapshotCache cache;

    assert(!cache.hasSnapshot());

    VdrSnapshot snapshot;
    cache.update(snapshot);

    assert(cache.hasSnapshot());
    assert(cache.hasSnapshotForBackend("default"));

    cache.clear();

    assert(!cache.hasSnapshot());
    assert(!cache.hasSnapshotForBackend("default"));
}

static void test_backend_snapshot_storage()
{
    SnapshotCache cache;

    VdrSnapshot first;
    first.status.host = "first-host";

    VdrSnapshot second;
    second.status.host = "second-host";

    cache.updateForBackend("default", first);
    cache.updateForBackend("ferienhaus", second);

    assert(cache.hasSnapshotForBackend("default"));
    assert(cache.hasSnapshotForBackend("ferienhaus"));

    const VdrSnapshot* defaultSnapshot =
        cache.snapshotForBackend("default");

    const VdrSnapshot* ferienhausSnapshot =
        cache.snapshotForBackend("ferienhaus");

    assert(defaultSnapshot != nullptr);
    assert(ferienhausSnapshot != nullptr);

    assert(defaultSnapshot->backendId == "default");
    assert(defaultSnapshot->status.host == "first-host");

    assert(ferienhausSnapshot->backendId == "ferienhaus");
    assert(ferienhausSnapshot->status.host == "second-host");

    assert(cache.backendIds().size() == 2);
}

static void test_clear_backend()
{
    SnapshotCache cache;

    VdrSnapshot snapshot;
    cache.updateForBackend("default", snapshot);

    assert(cache.hasSnapshotForBackend("default"));

    cache.clearBackend("default");

    assert(!cache.hasSnapshotForBackend("default"));
}

int main()
{
    test_single_snapshot_compatibility();
    test_backend_snapshot_storage();
    test_clear_backend();

    return 0;
}
