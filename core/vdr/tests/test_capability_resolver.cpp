#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrCapabilitySet emptyCapabilities;
    CapabilityResolver emptyResolver(emptyCapabilities);

    assert(!emptyResolver.supports("snapshot.read"));
    assert(!emptyResolver.supports("status.read"));
    assert(!emptyResolver.supports("health.read"));
    assert(!emptyResolver.supports("recordings.read"));
    assert(!emptyResolver.supports("timers.read"));
    assert(!emptyResolver.supports("channels.read"));
    assert(!emptyResolver.supports("events.read"));
    assert(!emptyResolver.supports("events.read.selective"));
    assert(!emptyResolver.supports("recordings.delete"));
    assert(!emptyResolver.supports("timers.create"));
    assert(!emptyResolver.supports("unknown.capability"));

    VdrCapabilitySet readOnlyCapabilities =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver readOnlyResolver(readOnlyCapabilities);

    assert(readOnlyResolver.supports("snapshot.read"));
    assert(readOnlyResolver.supports("status.read"));
    assert(readOnlyResolver.supports("health.read"));
    assert(readOnlyResolver.supports("recordings.read"));
    assert(readOnlyResolver.supports("timers.read"));
    assert(readOnlyResolver.supports("channels.read"));
    assert(readOnlyResolver.supports("events.read"));
    assert(readOnlyResolver.supports("events.read.selective"));

    assert(!readOnlyResolver.supports("recordings.delete"));
    assert(!readOnlyResolver.supports("timers.create"));
    assert(!readOnlyResolver.supports("unknown.capability"));

    std::cout
        << "test_capability_resolver passed"
        << std::endl;

    return 0;
}
