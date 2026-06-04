#include "SnapshotCacheService.h"

#include <cassert>

int main()
{
    SnapshotCache cache;
    SnapshotCacheService service(cache);

    assert(!service.cache().hasSnapshot());

    VdrSnapshot snapshot;
    service.cache().update(snapshot);

    assert(service.cache().hasSnapshot());

    service.cache().clear();

    assert(!service.cache().hasSnapshot());

    return 0;
}
