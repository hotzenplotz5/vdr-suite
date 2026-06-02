#include "MockVdrAdapter.h"
#include "VdrOverview.h"
#include "VdrOverviewService.h"
#include "VdrService.h"

#include <cassert>

static void test_vdr_overview_service_returns_status()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.status.enabled == true);
    assert(overview.status.mode == "mock");
    assert(overview.status.host == "mock");
    assert(overview.status.port == 0);
    assert(overview.status.state == "connected");
}

static void test_vdr_overview_service_counts_channels()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalChannels == 3);
    assert(overview.radioChannels == 1);
    assert(overview.encryptedChannels == 0);
}

static void test_vdr_overview_service_counts_events()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalEvents == 2);
}

static void test_vdr_overview_service_counts_timers()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalTimers == 1);
    assert(overview.activeTimers == 1);
    assert(overview.recordingTimers == 0);
}

static void test_vdr_overview_service_counts_recordings()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrOverviewService overviewService(vdrService);

    VdrOverview overview = overviewService.getOverview();

    assert(overview.totalRecordings == 2);
}

int main()
{
    test_vdr_overview_service_returns_status();
    test_vdr_overview_service_counts_channels();
    test_vdr_overview_service_counts_events();
    test_vdr_overview_service_counts_timers();
    test_vdr_overview_service_counts_recordings();

    return 0;
}
