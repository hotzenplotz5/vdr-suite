#include "IVdrAdapter.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class PreviewCacheRefreshCountingAdapter final : public IVdrAdapter
{
public:
    mutable int fullEventsReadCount = 0;
    mutable int selectiveEventsReadCount = 0;
    mutable VdrEventQuery lastQuery;
    bool throwOnSelectiveEvents = false;

    VdrStatus getStatus() const override
    {
        return VdrStatus();
    }

    std::vector<VdrEvent> getEvents() const override
    {
        ++fullEventsReadCount;
        return {};
    }

    std::vector<VdrEvent> getEvents(const VdrEventQuery& query) const override
    {
        ++selectiveEventsReadCount;
        lastQuery = query;

        if (throwOnSelectiveEvents) {
            throw std::runtime_error("selective EPG unavailable");
        }

        VdrEvent first;
        first.id = "event-1";
        first.title = "Preview Cache Event 1";

        VdrEvent second;
        second.id = "event-2";
        second.title = "Preview Cache Event 2";

        return {first, second};
    }

    std::vector<VdrChannel> getChannels() const override
    {
        return {};
    }

    std::vector<VdrTimer> getTimers() const override
    {
        return {};
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {};
    }

    VdrChangeState getChangeState() const override
    {
        return VdrChangeState();
    }
};

static void test_refresh_marks_backend_ready_with_selective_events()
{
    PreviewCacheRefreshCountingAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "home-vdr",
        nullptr,
        nullptr);
    SearchTimerPreviewEpgCache cache;
    SearchTimerPreviewEpgCacheRefreshService refreshService(cache, builder);

    SearchTimerPreviewEpgCacheRefreshRequest request;
    request.backendId = "home-vdr";
    request.from = 100;
    request.timespan = 7200;
    request.channelEventLimit = 25;

    const SearchTimerPreviewEpgCacheRefreshResult result =
        refreshService.refreshBackend(request);

    assert(result.backendId == "home-vdr");
    assert(result.status == "ready");
    assert(result.available == true);
    assert(result.eventCount == 2);

    assert(cache.statusForBackend("home-vdr") == SearchTimerPreviewEpgCacheStatus::Ready);
    assert(cache.readyEventCountForBackend("home-vdr") == 2);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
    assert(adapter.lastQuery.from == 100);
    assert(adapter.lastQuery.timespan == 7200);
    assert(adapter.lastQuery.channelEventLimit == 25);
}

static void test_default_request_uses_default_backend_and_bounded_channel_events()
{
    PreviewCacheRefreshCountingAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(service);
    SearchTimerPreviewEpgCache cache;
    SearchTimerPreviewEpgCacheRefreshService refreshService(cache, builder);

    SearchTimerPreviewEpgCacheRefreshRequest request;
    request.backendId = "";

    const SearchTimerPreviewEpgCacheRefreshResult result =
        refreshService.refreshBackend(request);

    assert(result.backendId == "default");
    assert(result.status == "ready");
    assert(result.available == true);
    assert(result.eventCount == 2);

    assert(cache.statusForBackend("default") == SearchTimerPreviewEpgCacheStatus::Ready);
    assert(cache.readyEventCountForBackend("default") == 2);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
    assert(adapter.lastQuery.channelEventLimit == 50);
}

static void test_refresh_marks_backend_unavailable_when_selective_events_fail()
{
    PreviewCacheRefreshCountingAdapter adapter;
    adapter.throwOnSelectiveEvents = true;

    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "remote-vdr",
        nullptr,
        nullptr);
    SearchTimerPreviewEpgCache cache;
    SearchTimerPreviewEpgCacheRefreshService refreshService(cache, builder);

    SearchTimerPreviewEpgCacheRefreshRequest request;
    request.backendId = "remote-vdr";
    request.channelEventLimit = 10;

    const SearchTimerPreviewEpgCacheRefreshResult result =
        refreshService.refreshBackend(request);

    assert(result.backendId == "remote-vdr");
    assert(result.status == "unavailable");
    assert(result.available == false);
    assert(result.eventCount == 0);

    assert(cache.statusForBackend("remote-vdr") == SearchTimerPreviewEpgCacheStatus::Unavailable);
    assert(cache.readyEventCountForBackend("remote-vdr") == 0);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
    assert(adapter.lastQuery.channelEventLimit == 10);
}

int main()
{
    test_refresh_marks_backend_ready_with_selective_events();
    test_default_request_uses_default_backend_and_bounded_channel_events();
    test_refresh_marks_backend_unavailable_when_selective_events_fail();

    std::cout << "test_search_timer_preview_epg_cache_refresh_service passed" << std::endl;
    return 0;
}
