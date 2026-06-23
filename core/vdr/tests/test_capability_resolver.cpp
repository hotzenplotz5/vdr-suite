#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

static void assertUnsupportedState(
    const CapabilityState& state,
    const std::string& capabilityName,
    const std::string& reason)
{
    assert(state.capabilityName() == capabilityName);
    assert(!state.supported());
    assert(state.availability() == CapabilityAvailability::Unsupported);
    assert(state.reason() == reason);
    assert(!state.availableNow());
}

static void assertAvailableState(
    const CapabilityState& state,
    const std::string& capabilityName)
{
    assert(state.capabilityName() == capabilityName);
    assert(state.supported());
    assert(state.availability() == CapabilityAvailability::Available);
    assert(state.reason().empty());
    assert(state.availableNow());
}

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
    assert(!emptyResolver.supports("epg.search.fuzzy.fallback"));
    assert(!emptyResolver.supports("epg.search.fuzzy.native"));
    assert(!emptyResolver.supports("recordings.delete"));
    assert(!emptyResolver.supports("timers.create"));
    assert(!emptyResolver.supports("unknown.capability"));

    assertUnsupportedState(
        emptyResolver.state("recordings.read"),
        "recordings.read",
        "capability unsupported by backend");
    assertUnsupportedState(
        emptyResolver.state("unknown.capability"),
        "unknown.capability",
        "unknown capability");

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
    assert(readOnlyResolver.supports("epg.search.fuzzy.fallback"));
    assert(!readOnlyResolver.supports("epg.search.fuzzy.native"));

    assertAvailableState(
        readOnlyResolver.state("recordings.read"),
        "recordings.read");
    assertAvailableState(
        readOnlyResolver.state("events.read.selective"),
        "events.read.selective");
    assertAvailableState(
        readOnlyResolver.state("epg.search.fuzzy.fallback"),
        "epg.search.fuzzy.fallback");
    assertUnsupportedState(
        readOnlyResolver.state("epg.search.fuzzy.native"),
        "epg.search.fuzzy.native",
        "capability unsupported by backend");

    assert(!readOnlyResolver.supports("recordings.delete"));
    assert(!readOnlyResolver.supports("timers.create"));
    assert(!readOnlyResolver.supports("unknown.capability"));
    assertUnsupportedState(
        readOnlyResolver.state("unknown.capability"),
        "unknown.capability",
        "unknown capability");

    std::cout
        << "test_capability_resolver passed"
        << std::endl;

    return 0;
}
