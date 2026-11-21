#include "CapabilityReport.h"

#include <cassert>
#include <iostream>
#include <vector>

int main()
{
    std::vector<CapabilityState> states;
    states.push_back(CapabilityState::available("recordings.read"));
    states.push_back(
        CapabilityState::unsupported(
            "timers.create",
            "capability unsupported by backend"));

    CapabilityReport report(
        "living-room",
        states);

    assert(report.backendId() == "living-room");
    assert(!report.empty());
    assert(report.size() == 2);
    assert(report.capabilities().at(0).capabilityName() == "recordings.read");
    assert(report.capabilities().at(0).availableNow());
    assert(report.capabilities().at(1).capabilityName() == "timers.create");
    assert(!report.capabilities().at(1).availableNow());

    CapabilityReport emptyReport(
        "empty",
        {});

    assert(emptyReport.backendId() == "empty");
    assert(emptyReport.empty());
    assert(emptyReport.size() == 0);

    std::cout
        << "test_capability_report passed"
        << std::endl;

    return 0;
}
