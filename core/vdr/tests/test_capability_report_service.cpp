#include "CapabilityReportService.h"
#include "CapabilityResolver.h"
#include "VdrCapabilitySet.h"

#include <cassert>
#include <iostream>

namespace
{
bool expectedSnapshotReadOnlyCapability(const std::string& capability)
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
    VdrCapabilitySet capabilities =
        VdrCapabilitySet::snapshotReadOnly();

    CapabilityResolver resolver(capabilities);
    CapabilityReportBuilder builder;

    CapabilityReportService service(
        "mock-backend",
        resolver,
        builder);

    CapabilityReport report = service.getReport();

    assert(report.backendId() == "mock-backend");
    assert(!report.empty());
    assert(report.size() == 20);

    bool sawRemoteControl = false;
    bool sawLiveOverlayRead = false;
    bool sawOsdView = false;
    bool sawOsdControl = false;
    bool sawManualSearch = false;
    bool sawManualAssignment = false;
    bool sawManualMovie = false;
    bool sawManualSeries = false;
    bool sawManualEpisode = false;

    for (const auto& state : report.capabilities())
    {
        const bool expected = expectedSnapshotReadOnlyCapability(
            state.capabilityName());
        assert(state.supported() == expected);
        assert(state.availableNow() == expected);
        assert(state.availability() ==
            (expected
                ? CapabilityAvailability::Available
                : CapabilityAvailability::Unsupported));

        sawRemoteControl = sawRemoteControl ||
            state.capabilityName() == "remote.control";
        sawLiveOverlayRead = sawLiveOverlayRead ||
            state.capabilityName() == "live.overlay.read";
        sawOsdView = sawOsdView || state.capabilityName() == "osd.view";
        sawOsdControl = sawOsdControl || state.capabilityName() == "osd.control";
        sawManualSearch = sawManualSearch ||
            state.capabilityName() == "metadata.recording.manualSearch";
        sawManualAssignment = sawManualAssignment ||
            state.capabilityName() == "metadata.recording.manualAssignment";
        sawManualMovie = sawManualMovie ||
            state.capabilityName() == "metadata.recording.manualAssignment.movie";
        sawManualSeries = sawManualSeries ||
            state.capabilityName() == "metadata.recording.manualAssignment.series";
        sawManualEpisode = sawManualEpisode ||
            state.capabilityName() == "metadata.recording.manualAssignment.episode";
    }

    assert(sawRemoteControl);
    assert(sawLiveOverlayRead);
    assert(sawOsdView);
    assert(sawOsdControl);
    assert(sawManualSearch);
    assert(sawManualAssignment);
    assert(sawManualMovie);
    assert(sawManualSeries);
    assert(sawManualEpisode);

    VdrCapabilitySet enabledCapabilities =
        VdrCapabilitySet::snapshotReadOnly();
    enabledCapabilities.searchTimerPreviewNative = true;
    enabledCapabilities.remoteControl = true;
    enabledCapabilities.liveOverlayRead = true;
    enabledCapabilities.osdView = true;
    enabledCapabilities.osdControl = false;
    CapabilityResolver enabledResolver(enabledCapabilities);

    assert(enabledResolver.state("searchtimer.preview.native").availableNow());
    assert(enabledResolver.state("remote.control").availableNow());
    assert(enabledResolver.state("live.overlay.read").availableNow());
    assert(enabledResolver.state("osd.view").availableNow());
    assert(!enabledResolver.state("osd.control").availableNow());
    assert(enabledResolver.state(
        "metadata.recording.manualSearch").availableNow());
    assert(enabledResolver.state(
        "metadata.recording.manualAssignment.episode").availableNow());

    VdrCapabilitySet emptyCapabilities;
    CapabilityResolver emptyResolver(emptyCapabilities);

    CapabilityReportService emptyService(
        "empty-backend",
        emptyResolver,
        builder);

    CapabilityReport emptyReport = emptyService.getReport();

    assert(emptyReport.backendId() == "empty-backend");
    assert(!emptyReport.empty());
    assert(emptyReport.size() == 20);

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
