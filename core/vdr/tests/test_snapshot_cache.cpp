#include "SnapshotCache.h"

#include <cassert>

int main()
{
    SnapshotCache cache;

    assert(!cache.hasSnapshot());

    VdrSnapshot snapshot;

    cache.update(snapshot);

    assert(cache.hasSnapshot());

    cache.clear();

    assert(!cache.hasSnapshot());

    return 0;
}
