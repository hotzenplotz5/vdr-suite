#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"

#include <cassert>

int main()
{
    SnapshotCache cache;
    SnapshotCacheService cacheService(cache);
    SnapshotAccessService accessService(cacheService);

    assert(!accessService.hasSnapshot());
    assert(accessService.snapshot() == nullptr);

    return 0;
}
