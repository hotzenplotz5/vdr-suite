#include "CapabilityState.h"

#include <cassert>
#include <iostream>

int main()
{
    CapabilityState available =
        CapabilityState::available("recordings.read");

    assert(available.capabilityName() == "recordings.read");
    assert(available.supported());
    assert(available.availability() == CapabilityAvailability::Available);
    assert(available.reason().empty());
    assert(available.availableNow());

    CapabilityState unsupported =
        CapabilityState::unsupported(
            "live.stream",
            "backend does not support live streaming");

    assert(unsupported.capabilityName() == "live.stream");
    assert(!unsupported.supported());
    assert(unsupported.availability() == CapabilityAvailability::Unsupported);
    assert(unsupported.reason() == "backend does not support live streaming");
    assert(!unsupported.availableNow());

    CapabilityState notConfigured =
        CapabilityState::notConfigured(
            "recordings.read",
            "no backend configured");

    assert(!notConfigured.supported());
    assert(notConfigured.availability() == CapabilityAvailability::NotConfigured);
    assert(notConfigured.reason() == "no backend configured");
    assert(!notConfigured.availableNow());

    CapabilityState offline =
        CapabilityState::offline(
            "timers.read",
            "backend offline");

    assert(offline.supported());
    assert(offline.availability() == CapabilityAvailability::Offline);
    assert(offline.reason() == "backend offline");
    assert(!offline.availableNow());

    CapabilityState temporarilyUnavailable =
        CapabilityState::temporarilyUnavailable(
            "events.read",
            "backend refresh in progress");

    assert(temporarilyUnavailable.supported());
    assert(temporarilyUnavailable.availability() == CapabilityAvailability::TemporarilyUnavailable);
    assert(temporarilyUnavailable.reason() == "backend refresh in progress");
    assert(!temporarilyUnavailable.availableNow());

    std::cout
        << "test_capability_state passed"
        << std::endl;

    return 0;
}
