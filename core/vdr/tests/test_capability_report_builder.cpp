#include "CapabilityReportBuilder.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrCapabilitySet emptyCapabilities;
    CapabilityResolver emptyResolver(emptyCapabilities);
    CapabilityReportBuilder builder;

    CapabilityReport emptyReport =
        builder.build(
            "empty-backend",
            emptyResolver);

    assert(emptyReport.backendId() == "empty-backend");
    assert(!emptyReport.empty());
    assert(emptyReport.size() == 8);

    for (const auto& state : emptyReport.capabilities())
    {
        assert(!state.supported());
        assert(!state.availableNow());
        assert(state.availability() == CapabilityAvailability::Unsupported);
    }

    VdrCapabilitySet readOnlyCapabilities =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver readOnlyResolver(readOnlyCapabilities);

    CapabilityReport readOnlyReport =
        builder.build(
            "mock-backend",
            readOnlyResolver);

    assert(readOnlyReport.backendId() == "mock-backend");
    assert(!readOnlyReport.empty());
    assert(readOnlyReport.size() == 8);

    for (const auto& state : readOnlyReport.capabilities())
    {
        assert(state.supported());
        assert(state.availableNow());
        assert(state.availability() == CapabilityAvailability::Available);
    }

    assert(readOnlyReport.capabilities().at(0).capabilityName() == "snapshot.read");
    assert(readOnlyReport.capabilities().at(3).capabilityName() == "recordings.read");
    assert(readOnlyReport.capabilities().at(7).capabilityName() == "events.read.selective");

    std::cout
        << "test_capability_report_builder passed"
        << std::endl;

    return 0;
}
