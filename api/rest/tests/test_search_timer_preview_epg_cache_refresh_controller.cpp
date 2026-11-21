#include "IVdrAdapter.h"
#include "SearchTimerPreviewEpgCacheRefreshController.h"
#include "SearchTimerPreviewEpgCacheRefreshService.h"
#include "SearchTimerPreviewEpgCacheRefreshServiceRegistry.h"
#include "VdrService.h"
#include "VdrSnapshotBuilder.h"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class RefreshControllerCountingAdapter final : public IVdrAdapter
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

        VdrEvent event;
        event.id = "event-1";
        event.title = "Controller Event";
        return {event};
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

static void test_controller_refreshes_registered_backend()
{
    RefreshControllerCountingAdapter adapter;
    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "default",
        nullptr,
        nullptr);
    SearchTimerPreviewEpgCache cache;
    SearchTimerPreviewEpgCacheRefreshService refreshService(cache, builder);
    SearchTimerPreviewEpgCacheRefreshServiceRegistry registry;
    registry.registerService("default", refreshService);

    SearchTimerPreviewEpgCacheRefreshController controller(registry);

    const ApiResponse response =
        controller.refreshBackend("", 100, 7200, -1, 0, 33);

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"status\":\"ready\"") != std::string::npos);
    assert(response.body.find("\"available\":true") != std::string::npos);
    assert(response.body.find("\"eventCount\":1") != std::string::npos);

    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
    assert(adapter.lastQuery.from == 100);
    assert(adapter.lastQuery.timespan == 7200);
    assert(adapter.lastQuery.channelEventLimit == 33);
}

static void test_controller_returns_not_found_for_unknown_backend()
{
    SearchTimerPreviewEpgCacheRefreshServiceRegistry registry;
    SearchTimerPreviewEpgCacheRefreshController controller(registry);

    const ApiResponse response =
        controller.refreshBackend("missing", -1, 0, -1, 0, 50);

    assert(response.statusCode == 404);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"missing\"") != std::string::npos);
    assert(response.body.find("\"status\":\"backend-not-found\"") != std::string::npos);
    assert(response.body.find("\"available\":false") != std::string::npos);
    assert(response.body.find("\"eventCount\":0") != std::string::npos);
}

static void test_controller_returns_unavailable_when_refresh_fails()
{
    RefreshControllerCountingAdapter adapter;
    adapter.throwOnSelectiveEvents = true;

    VdrService service(adapter);
    VdrSnapshotBuilder builder(
        service,
        "remote",
        nullptr,
        nullptr);
    SearchTimerPreviewEpgCache cache;
    SearchTimerPreviewEpgCacheRefreshService refreshService(cache, builder);
    SearchTimerPreviewEpgCacheRefreshServiceRegistry registry;
    registry.registerService("remote", refreshService);

    SearchTimerPreviewEpgCacheRefreshController controller(registry);

    const ApiResponse response =
        controller.refreshBackend("remote", -1, 0, -1, 0, 50);

    assert(response.statusCode == 503);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"backendId\":\"remote\"") != std::string::npos);
    assert(response.body.find("\"status\":\"unavailable\"") != std::string::npos);
    assert(response.body.find("\"available\":false") != std::string::npos);
    assert(response.body.find("\"eventCount\":0") != std::string::npos);

    assert(cache.statusForBackend("remote") == SearchTimerPreviewEpgCacheStatus::Unavailable);
    assert(adapter.fullEventsReadCount == 0);
    assert(adapter.selectiveEventsReadCount == 1);
}

int main()
{
    test_controller_refreshes_registered_backend();
    test_controller_returns_not_found_for_unknown_backend();
    test_controller_returns_unavailable_when_refresh_fails();

    std::cout << "test_search_timer_preview_epg_cache_refresh_controller passed" << std::endl;
    return 0;
}
