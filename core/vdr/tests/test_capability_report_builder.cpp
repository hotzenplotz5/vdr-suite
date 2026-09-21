#include "CapabilityReportBuilder.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
bool expectedReadOnlyCapability(
    const std::string& capability)
{
    return capability != "epg.search.fuzzy.native" &&
        capability != "searchtimer.preview.native" &&
        capability != "remote.control" &&
        capability != "live.overlay.read" &&
        capability != "osd.view" &&
        capability != "osd.control";
}
}

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
    assert(emptyReport.size() == 20);

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
    assert(readOnlyReport.size() == 20);

    for (const auto& state : readOnlyReport.capabilities())
    {
        if (expectedReadOnlyCapability(state.capabilityName()))
        {
            assert(state.supported());
            assert(state.availableNow());
            assert(state.availability() == CapabilityAvailability::Available);
        }
        else
        {
            assert(!state.supported());
            assert(!state.availableNow());
            assert(state.availability() == CapabilityAvailability::Unsupported);
        }
    }

    assert(readOnlyReport.capabilities().at(0).capabilityName() == "snapshot.read");
    assert(readOnlyReport.capabilities().at(3).capabilityName() == "recordings.read");
    assert(readOnlyReport.capabilities().at(7).capabilityName() == "events.read.selective");
    assert(readOnlyReport.capabilities().at(8).capabilityName() == "epg.search.fuzzy.fallback");
    assert(readOnlyReport.capabilities().at(9).capabilityName() == "epg.search.fuzzy.native");
    assert(readOnlyReport.capabilities().at(10).capabilityName() == "searchtimer.preview.native");
    assert(readOnlyReport.capabilities().at(11).capabilityName() == "remote.control");
    assert(readOnlyReport.capabilities().at(12).capabilityName() == "live.overlay.read");
    assert(readOnlyReport.capabilities().at(13).capabilityName() == "osd.view");
    assert(readOnlyReport.capabilities().at(14).capabilityName() == "osd.control");
    assert(readOnlyReport.capabilities().at(15).capabilityName() ==
        "metadata.recording.manualSearch");
    assert(readOnlyReport.capabilities().at(16).capabilityName() ==
        "metadata.recording.manualAssignment");
    assert(readOnlyReport.capabilities().at(17).capabilityName() ==
        "metadata.recording.manualAssignment.movie");
    assert(readOnlyReport.capabilities().at(18).capabilityName() ==
        "metadata.recording.manualAssignment.series");
    assert(readOnlyReport.capabilities().at(19).capabilityName() ==
        "metadata.recording.manualAssignment.episode");

    std::cout
        << "test_capability_report_builder passed"
        << std::endl;

    return 0;
}
