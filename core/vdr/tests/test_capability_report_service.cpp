#include "CapabilityReportService.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrCapabilitySet capabilities =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver resolver(capabilities);
    CapabilityReportBuilder builder;

    CapabilityReportService service(
        "mock-backend",
        resolver,
        builder);

    CapabilityReport report =
        service.getReport();

    assert(report.backendId() == "mock-backend");
    assert(!report.empty());
    assert(report.size() == 8);

    for (const auto& state : report.capabilities())
    {
        assert(state.supported());
        assert(state.availableNow());
        assert(state.availability() == CapabilityAvailability::Available);
    }

    VdrCapabilitySet emptyCapabilities;
    CapabilityResolver emptyResolver(emptyCapabilities);

    CapabilityReportService emptyService(
        "empty-backend",
        emptyResolver,
        builder);

    CapabilityReport emptyReport =
        emptyService.getReport();

    assert(emptyReport.backendId() == "empty-backend");
    assert(!emptyReport.empty());
    assert(emptyReport.size() == 8);

    for (const auto& state : emptyReport.capabilities())
    {
        assert(!state.supported());
        assert(!state.availableNow());
        assert(state.availability() == CapabilityAvailability::Unsupported);
    }

    std::cout
        << "test_capability_report_service passed"
        << std::endl;

    return 0;
}
