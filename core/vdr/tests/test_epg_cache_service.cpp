#include "Database.h"
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
        return VdrChangeState{};
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
    event.contentDescriptors = {"cache", "test"};

    return event;
}

static void test_unbounded_refresh_is_rejected_without_fetching_events()
{
    std::remove("/tmp/vdr-suite-epg-cache-service-unbounded-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-service-unbounded-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    adapter.events = {
        make_event("event-1", "channel-1", "Should Not Store", "0900", "1000")};

    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);

    VdrEventQuery query;

    const EpgCacheRefreshResult result =
        service.refreshBackendWindow("home-vdr", query);

    assert(!result.accepted);
    assert(!result.fetched);
    assert(!result.stored);
    assert(result.eventCount == 0);
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 0);
    assert(service.countForBackend("home-vdr") == 0);
}

static void test_bounded_refresh_fetches_and_stores_backend_scoped_events()
{
    std::remove("/tmp/vdr-suite-epg-cache-service-refresh-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-service-refresh-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);

    VdrEventQuery query;
    query.channelId = "channel-1";
    query.from = 0;
    query.channelEventLimit = 2;

    adapter.events = {
        make_event("event-1", "channel-1", "Home Current", "0900", "1100"),
        make_event("event-2", "channel-1", "Home Next", "1200", "1300")};

    const EpgCacheRefreshResult homeResult =
        service.refreshBackendWindow("home-vdr", query);

    assert(homeResult.accepted);
    assert(homeResult.fetched);
    assert(homeResult.stored);
    assert(homeResult.eventCount == 2);
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 1);
    assert(adapter.lastQuery.channelId == "channel-1");
    assert(adapter.lastQuery.channelEventLimit == 2);

    adapter.events = {
        make_event("event-1", "channel-1", "Remote Current", "0900", "1100")};

    const EpgCacheRefreshResult remoteResult =
        service.refreshBackendWindow("parents-vdr", query);

    assert(remoteResult.accepted);
    assert(remoteResult.fetched);
    assert(remoteResult.stored);
    assert(remoteResult.eventCount == 1);
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 2);

    assert(service.countForBackend("home-vdr") == 2);
    assert(service.countForBackend("parents-vdr") == 1);

    const std::vector<VdrEvent> homeEvents =
        service.findNowNextForBackend("home-vdr", "channel-1", "1000", 5);
    const std::vector<VdrEvent> remoteEvents =
        service.findNowNextForBackend("parents-vdr", "channel-1", "1000", 5);

    assert(homeEvents.size() == 2);
    assert(homeEvents.at(0).title == "Home Current");
    assert(homeEvents.at(1).title == "Home Next");

    assert(remoteEvents.size() == 1);
    assert(remoteEvents.at(0).title == "Remote Current");
}

static void test_cache_reads_do_not_fetch_adapter_events()
{
    std::remove("/tmp/vdr-suite-epg-cache-service-read-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-cache-service-read-test.db"));

    EpgEventRepository repository(database);
    MockEventAdapter adapter;
    VdrService vdrService(adapter);
    EpgCacheService service(repository, vdrService);

    VdrEventQuery query;
    query.limit = 1;

    adapter.events = {
        make_event("event-1", "channel-1", "Cached Event", "0900", "1100")};

    const EpgCacheRefreshResult result =
        service.refreshBackendWindow("home-vdr", query);

    assert(result.accepted);
    assert(adapter.boundedEventCalls == 1);

    const std::vector<VdrEvent> cached =
        service.findWindowForBackend("home-vdr", "channel-1", "1000", "1200", 5);

    assert(cached.size() == 1);
    assert(cached.at(0).title == "Cached Event");
    assert(adapter.unboundedEventCalls == 0);
    assert(adapter.boundedEventCalls == 1);
}

static void test_only_count_refresh_is_rejected()
{
    VdrEventQuery query;
    query.onlyCount = true;
    query.limit = 10;

    assert(!EpgCacheService::isBoundedRefreshQuery(query));
}

int main()
{
    test_unbounded_refresh_is_rejected_without_fetching_events();
    test_bounded_refresh_fetches_and_stores_backend_scoped_events();
    test_cache_reads_do_not_fetch_adapter_events();
    test_only_count_refresh_is_rejected();

    return 0;
}
