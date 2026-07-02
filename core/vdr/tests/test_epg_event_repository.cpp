#include "Database.h"
#include "EpgEventRepository.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

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
    event.durationSeconds = 100;
    event.parentalRating = 0;
    event.contentDescriptors = {"news", "test"};

    return event;
}

static void test_repository_is_backend_scoped()
{
    std::remove("/tmp/vdr-suite-epg-event-repository-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-event-repository-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("epg_events"));

    const VdrEvent homeCurrent =
        make_event("event-1", "channel-1", "Home Current", "0900", "1100");
    const VdrEvent homeNext =
        make_event("event-2", "channel-1", "Home Next", "1200", "1300");
    const VdrEvent remoteCurrent =
        make_event("event-1", "channel-1", "Remote Current", "0900", "1100");

    assert(repository.upsertEventsForBackend(
        "home-vdr",
        std::vector<VdrEvent>{homeCurrent, homeNext}));
    assert(repository.upsertEventsForBackend(
        "parents-vdr",
        std::vector<VdrEvent>{remoteCurrent}));

    assert(repository.countForBackend("home-vdr") == 2);
    assert(repository.countForBackend("parents-vdr") == 1);

    const std::vector<VdrEvent> homeNowNext =
        repository.findNowNextForBackend(
            "home-vdr",
            "channel-1",
            "1000",
            2);

    assert(homeNowNext.size() == 2);
    assert(homeNowNext.at(0).id == "event-1");
    assert(homeNowNext.at(0).title == "Home Current");
    assert(homeNowNext.at(1).id == "event-2");
    assert(homeNowNext.at(1).title == "Home Next");

    const std::vector<VdrEvent> remoteNowNext =
        repository.findNowNextForBackend(
            "parents-vdr",
            "channel-1",
            "1000",
            2);

    assert(remoteNowNext.size() == 1);
    assert(remoteNowNext.at(0).id == "event-1");
    assert(remoteNowNext.at(0).title == "Remote Current");
}

static void test_upsert_updates_only_matching_backend()
{
    std::remove("/tmp/vdr-suite-epg-event-repository-upsert-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-event-repository-upsert-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());

    assert(repository.upsertEventsForBackend(
        "home-vdr",
        std::vector<VdrEvent>{
            make_event("event-1", "channel-1", "Home Original", "0900", "1100")}));
    assert(repository.upsertEventsForBackend(
        "parents-vdr",
        std::vector<VdrEvent>{
            make_event("event-1", "channel-1", "Remote Original", "0900", "1100")}));

    assert(repository.upsertEventsForBackend(
        "home-vdr",
        std::vector<VdrEvent>{
            make_event("event-1", "channel-1", "Home Updated", "0900", "1100")}));

    const std::vector<VdrEvent> homeEvents =
        repository.findNowNextForBackend("home-vdr", "channel-1", "1000", 5);
    const std::vector<VdrEvent> remoteEvents =
        repository.findNowNextForBackend("parents-vdr", "channel-1", "1000", 5);

    assert(repository.countForBackend("home-vdr") == 1);
    assert(repository.countForBackend("parents-vdr") == 1);

    assert(homeEvents.size() == 1);
    assert(homeEvents.at(0).title == "Home Updated");

    assert(remoteEvents.size() == 1);
    assert(remoteEvents.at(0).title == "Remote Original");
}

static void test_window_and_cleanup_are_backend_scoped()
{
    std::remove("/tmp/vdr-suite-epg-event-repository-window-test.db");

    Database database;
    assert(database.open("/tmp/vdr-suite-epg-event-repository-window-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());

    assert(repository.upsertEventsForBackend(
        "home-vdr",
        std::vector<VdrEvent>{
            make_event("old", "channel-1", "Old", "0800", "0900"),
            make_event("current", "channel-1", "Current", "0900", "1100"),
            make_event("next", "channel-1", "Next", "1200", "1300")}));

    assert(repository.upsertEventsForBackend(
        "parents-vdr",
        std::vector<VdrEvent>{
            make_event("remote-current", "channel-1", "Remote Current", "0900", "1100")}));

    const std::vector<VdrEvent> window =
        repository.findWindowForBackend(
            "home-vdr",
            "channel-1",
            "1000",
            "1150",
            10);

    assert(window.size() == 1);
    assert(window.at(0).id == "current");

    assert(repository.deleteExpiredForBackend("home-vdr", "1150"));
    assert(repository.countForBackend("home-vdr") == 1);
    assert(repository.countForBackend("parents-vdr") == 1);

    const std::vector<VdrEvent> remainingHome =
        repository.findNowNextForBackend(
            "home-vdr",
            "channel-1",
            "1000",
            10);

    assert(remainingHome.size() == 1);
    assert(remainingHome.at(0).id == "next");
}

int main()
{
    test_repository_is_backend_scoped();
    test_upsert_updates_only_matching_backend();
    test_window_and_cleanup_are_backend_scoped();

    return 0;
}
