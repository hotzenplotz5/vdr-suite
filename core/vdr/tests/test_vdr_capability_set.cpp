#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrCapabilitySet emptyCapabilities;

    assert(!emptyCapabilities.snapshotRead);
    assert(!emptyCapabilities.statusRead);
    assert(!emptyCapabilities.healthRead);
    assert(!emptyCapabilities.recordingsRead);
    assert(!emptyCapabilities.timersRead);
    assert(!emptyCapabilities.channelsRead);
    assert(!emptyCapabilities.eventsRead);
    assert(!emptyCapabilities.eventsSelectiveRead);
    assert(!emptyCapabilities.epgSearchFuzzyFallback);
    assert(!emptyCapabilities.epgSearchFuzzyNative);

    VdrCapabilitySet readOnlyCapabilities =
        VdrCapabilitySet::snapshotReadOnly();

    assert(readOnlyCapabilities.snapshotRead);
    assert(readOnlyCapabilities.statusRead);
    assert(readOnlyCapabilities.healthRead);
    assert(readOnlyCapabilities.recordingsRead);
    assert(readOnlyCapabilities.timersRead);
    assert(readOnlyCapabilities.channelsRead);
    assert(readOnlyCapabilities.eventsRead);
    assert(readOnlyCapabilities.eventsSelectiveRead);
    assert(readOnlyCapabilities.epgSearchFuzzyFallback);
    assert(!readOnlyCapabilities.epgSearchFuzzyNative);

    std::cout
        << "test_vdr_capability_set passed"
        << std::endl;

    return 0;
}
