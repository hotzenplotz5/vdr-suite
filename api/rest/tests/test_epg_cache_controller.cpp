#include "Database.h"
#include "EpgCacheController.h"
#include "EpgCacheService.h"
#include "IVdrAdapter.h"
#include "VdrService.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

class MockEventAdapter : public IVdrAdapter
{
public:
    mutable int unboundedEventCalls = 0;
    mutable int boundedEventCalls = 0;
    mutable VdrEventQuery lastQuery;

    std::vector<VdrEvent> events;

    VdrStatus getStatus() const override
    {
        return VdrStatus{};
    }

    std::vector<VdrEvent> getEvents() const override
    {
        ++unboundedEventCalls;
        return events;
    }

    std::vector<VdrEvent> getEvents(
        const VdrEventQuery& query) const override
    {
        ++boundedEventCalls;
        lastQuery = query;
        return events;
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
        assert(false && "EPG cache controller test must not read change state");
        __builtin_unreachable();
    }
};

static VdrEvent make_event(
    const std::string& id,
    const std::string& channelId,
    const std::string& title,
    const std::string& startTime,
    const std::string& endTime)
{
    VdrEvent event;

    event.id = id;
    event.channelId = channelId;
    event.title = title;
    event.subtitle = "subtitle-" + title;
    event.description = "description-" + title;
    event.startTime = startTime;
    event.endTime = endTime;
    event.durationSeconds = 1800;
    event.parentalRating = 0;

    return event;
}

static bool contains(
    const std::string& text,
    const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

static void test_unbounded_refresh_returns_bad_request_without_adapter_fetch()
{
    std::remove("/tmp/vdr-suite-epg-cache-controller-unbounded-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-controller-unbounded-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);
    EpgCacheController controller(service);

    VdrEventQuery query;

    const ApiResponse response =
        controller.refreshBackendWindow("home-vdr", query);

    assert(response.statusCode == 400);
    assert(response.contentType == "application/json");
    assert(contains(response.body, "\"backendId\":\"home-vdr\""));
    assert(contains(response.body, "\"accepted\":false"));
    assert(contains(response.body, "\"fetched\":false"));
    assert(contains(response.body, "\"stored\":false"));
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 0);
}

static void test_bounded_refresh_and_now_next_read_return_backend_scoped_json()
{
    std::remove("/tmp/vdr-suite-epg-cache-controller-refresh-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-controller-refresh-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);
    EpgCacheController controller(service);

    VdrEventQuery query;
    query.channelId = "channel-1";
    query.from = 0;
    query.channelEventLimit = 2;

    adapter.events = {
        make_event("event-1", "channel-1", "Home Current", "0900", "1100"),
        make_event("event-2", "channel-1", "Home Next", "1200", "1300")};

    const ApiResponse homeRefresh =
        controller.refreshBackendWindow("home-vdr", query);

    assert(homeRefresh.statusCode == 200);
    assert(contains(homeRefresh.body, "\"backendId\":\"home-vdr\""));
    assert(contains(homeRefresh.body, "\"accepted\":true"));
    assert(contains(homeRefresh.body, "\"fetched\":true"));
    assert(contains(homeRefresh.body, "\"stored\":true"));
    assert(contains(homeRefresh.body, "\"eventCount\":2"));
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 1);

    adapter.events = {
        make_event("event-1", "channel-1", "Remote Current", "0900", "1100")};

    const ApiResponse remoteRefresh =
        controller.refreshBackendWindow("parents-vdr", query);

    assert(remoteRefresh.statusCode == 200);
    assert(contains(remoteRefresh.body, "\"backendId\":\"parents-vdr\""));
    assert(contains(remoteRefresh.body, "\"eventCount\":1"));
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 2);

    const ApiResponse homeNowNext =
        controller.getNowNext("home-vdr", "channel-1", "1000", 5);
    const ApiResponse remoteNowNext =
        controller.getNowNext("parents-vdr", "channel-1", "1000", 5);

    assert(homeNowNext.statusCode == 200);
    assert(contains(homeNowNext.body, "\"backendId\":\"home-vdr\""));
    assert(contains(homeNowNext.body, "\"eventCount\":2"));
    assert(contains(homeNowNext.body, "Home Current"));
    assert(contains(homeNowNext.body, "Home Next"));
    assert(!contains(homeNowNext.body, "Remote Current"));

    assert(remoteNowNext.statusCode == 200);
    assert(contains(remoteNowNext.body, "\"backendId\":\"parents-vdr\""));
    assert(contains(remoteNowNext.body, "\"eventCount\":1"));
    assert(contains(remoteNowNext.body, "Remote Current"));
    assert(!contains(remoteNowNext.body, "Home Current"));

    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 2);
}

static void test_status_reports_count_and_last_refresh_metadata()
{
    std::remove("/tmp/vdr-suite-epg-cache-controller-status-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-controller-status-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);
    EpgCacheController controller(service);

    const ApiResponse initialStatus =
        controller.getStatus("default");

    assert(initialStatus.statusCode == 200);
    assert(contains(initialStatus.body, "\"backendId\":\"default\""));
    assert(contains(initialStatus.body, "\"ready\":false"));
    assert(contains(initialStatus.body, "\"eventCount\":0"));
    assert(contains(initialStatus.body, "\"lastRefreshKnown\":false"));

    VdrEventQuery query;
    query.limit = 1;

    adapter.events = {
        make_event("event-1", "channel-1", "Status Cached", "0900", "1100")};

    const ApiResponse refresh =
        controller.refreshBackendWindow("default", query);

    assert(refresh.statusCode == 200);

    const ApiResponse status =
        controller.getStatus("default");

    assert(status.statusCode == 200);
    assert(contains(status.body, "\"backendId\":\"default\""));
    assert(contains(status.body, "\"ready\":true"));
    assert(contains(status.body, "\"eventCount\":1"));
    assert(contains(status.body, "\"lastRefreshKnown\":true"));
    assert(contains(status.body, "\"lastRefreshAccepted\":true"));
    assert(contains(status.body, "\"lastRefreshFetched\":true"));
    assert(contains(status.body, "\"lastRefreshStored\":true"));
    assert(contains(status.body, "\"lastRefreshEventCount\":1"));
    assert(contains(status.body, "\"lastRefreshDurationMs\":"));
    assert(contains(status.body, "\"lastError\":\"\""));
}


static void test_window_read_defaults_empty_backend_to_default()
{
    std::remove("/tmp/vdr-suite-epg-cache-controller-default-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-controller-default-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);
    EpgCacheController controller(service);

    VdrEventQuery query;
    query.limit = 1;

    adapter.events = {
        make_event("event-1", "channel-1", "Default Cached", "0900", "1100")};

    const ApiResponse refresh =
        controller.refreshBackendWindow("", query);

    assert(refresh.statusCode == 200);
    assert(contains(refresh.body, "\"backendId\":\"default\""));

    const ApiResponse window =
        controller.getWindow("", "channel-1", "1000", "1200", 5);

    assert(window.statusCode == 200);
    assert(contains(window.body, "\"backendId\":\"default\""));
    assert(contains(window.body, "Default Cached"));
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 1);
}

int main()
{
    test_unbounded_refresh_returns_bad_request_without_adapter_fetch();
    test_bounded_refresh_and_now_next_read_return_backend_scoped_json();
    test_status_reports_count_and_last_refresh_metadata();
    test_window_read_defaults_empty_backend_to_default();

    return 0;
}
