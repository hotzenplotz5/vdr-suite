#include "Database.h"
#include "EpgEventRepository.h"

#include <sqlite3.h>

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


static int scalar_int(Database& database, const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(),
        sql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

static std::string scalar_text(
    Database& database,
    const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(),
        sql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);

    const unsigned char* value =
        sqlite3_column_text(statement, 0);

    const std::string result =
        value == nullptr
            ? std::string()
            : std::string(
                reinterpret_cast<const char*>(value));

    sqlite3_finalize(statement);
    return result;
}

static void test_unchanged_upsert_does_not_rewrite_row()
{
    const std::string filename =
        "/tmp/vdr-suite-epg-event-noop-upsert-test.db";

    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());

    const VdrEvent original = make_event(
        "event-1",
        "channel-1",
        "Original",
        "1000",
        "2000");

    assert(repository.upsertEventsForBackend(
        "default",
        {original}));

    assert(database.execute(
        "UPDATE epg_events "
        "SET updated_at='sentinel' "
        "WHERE backend_id='default' "
        "AND channel_id='channel-1' "
        "AND event_id='event-1';"));

    const int unchangedChangesBefore =
        sqlite3_total_changes(database.handle());

    assert(repository.upsertEventsForBackend(
        "default",
        {original}));

    assert(
        sqlite3_total_changes(database.handle()) ==
        unchangedChangesBefore);

    assert(scalar_text(
        database,
        "SELECT updated_at FROM epg_events "
        "WHERE backend_id='default' "
        "AND channel_id='channel-1' "
        "AND event_id='event-1';") == "sentinel");

    VdrEvent changed = original;
    changed.title = "Changed";

    const int changedChangesBefore =
        sqlite3_total_changes(database.handle());

    assert(repository.upsertEventsForBackend(
        "default",
        {changed}));

    assert(
        sqlite3_total_changes(database.handle()) ==
        changedChangesBefore + 1);

    assert(scalar_text(
        database,
        "SELECT title FROM epg_events "
        "WHERE backend_id='default' "
        "AND channel_id='channel-1' "
        "AND event_id='event-1';") == "Changed");

    assert(scalar_text(
        database,
        "SELECT updated_at FROM epg_events "
        "WHERE backend_id='default' "
        "AND channel_id='channel-1' "
        "AND event_id='event-1';") != "sentinel");
}

static bool query_plan_contains(
    Database& database,
    const std::string& sql,
    const std::string& expected)
{
    const std::string explainSql =
        "EXPLAIN QUERY PLAN " + sql;

    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(),
        explainSql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);

    bool found = false;
    int result = SQLITE_ROW;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const unsigned char* detail =
            sqlite3_column_text(statement, 3);

        if (detail != nullptr &&
            std::string(
                reinterpret_cast<const char*>(detail))
                .find(expected) != std::string::npos)
        {
            found = true;
        }
    }

    assert(result == SQLITE_DONE);
    assert(sqlite3_finalize(statement) == SQLITE_OK);
    return found;
}

static void test_integer_window_uses_end_epoch_index()
{
    const std::string filename =
        "/tmp/vdr-suite-epg-event-epoch-index-test.db";

    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());

    assert(query_plan_contains(
        database,
        "SELECT channel_id,event_id,start_time,end_time,"
        "content_descriptors "
        "FROM epg_events "
        "WHERE backend_id='default' "
        "AND CAST(end_time AS INTEGER)>1000 "
        "AND CAST(start_time AS INTEGER)<2000 "
        "ORDER BY CAST(start_time AS INTEGER),"
        "channel_id,event_id;",
        "idx_epg_events_backend_end_epoch"));
}

static void test_authoritative_window_removes_only_missing_native_ids()
{
    std::remove("/tmp/vdr-suite-epg-authoritative-window-test.db");

    Database database;
    assert(database.open(
        "/tmp/vdr-suite-epg-authoritative-window-test.db"));

    EpgEventRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.execute(
        "CREATE TABLE epg_event_artwork("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,"
        "width INTEGER,height INTEGER,resolved_at INTEGER,updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_cache("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,public_json TEXT,"
        "resolved_at INTEGER,updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_images("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,kind TEXT,image_index INTEGER,"
        "provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,"
        "updated_at TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id,kind,image_index));"
        "CREATE TABLE epg_scraper_metadata_people("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,ordinal INTEGER,role TEXT,"
        "name TEXT,name_folded TEXT,character_name TEXT,character_name_folded TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id,ordinal));"
        "CREATE TABLE suite_metadata_targets("
        "metadata_target_id TEXT PRIMARY KEY,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_target_bindings("
        "metadata_target_id TEXT PRIMARY KEY,target_type TEXT,backend_id TEXT,"
        "resource_key TEXT,native_id TEXT,channel_id TEXT,start_time INTEGER,"
        "end_time INTEGER,lifecycle_state TEXT,updated_at TEXT);"
        "CREATE TABLE suite_metadata_genre_assignments("
        "metadata_target_id TEXT,assignment_state TEXT,updated_at TEXT);"));

    const VdrEvent oldEvent = make_event(
        "38845", "channel-1", "The Great Wall", "1000", "2000");
    const VdrEvent currentEvent = make_event(
        "39568", "channel-1", "The Great Wall", "1000", "2000");
    const VdrEvent outsideWindow = make_event(
        "outside", "channel-1", "Later", "3000", "4000");
    const VdrEvent otherChannel = make_event(
        "other", "channel-2", "Other", "1000", "2000");

    assert(repository.upsertEventsForBackend(
        "default",
        {oldEvent, outsideWindow, otherChannel}));
    assert(database.execute(
        "INSERT INTO epg_event_artwork VALUES("
        "'default','channel-1','38845','tvscraper','/tmp/old.jpg',1280,720,1,'old');"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'default','channel-1','38845','{}',1,'old');"
        "INSERT INTO epg_scraper_metadata_images VALUES("
        "'default','channel-1','38845','preferred',0,'tvscraper','/tmp/old.jpg',"
        "1280,720,1,'old');"
        "INSERT INTO epg_scraper_metadata_people VALUES("
        "'default','channel-1','38845',0,'actor','John Travolta','john travolta',"
        "'Vincent Vega','vincent vega');"
        "INSERT INTO suite_metadata_targets VALUES('target-old','active','old');"
        "INSERT INTO suite_metadata_target_bindings VALUES("
        "'target-old','program-event','default','channel-1\\n38845','38845',"
        "'channel-1',1000,2000,'active','old');"
        "INSERT INTO suite_metadata_genre_assignments VALUES("
        "'target-old','missing','old');"));

    const EpgAuthoritativeWindowResult result =
        repository.replaceAuthoritativeWindowForBackend(
            "default",
            "900",
            "2100",
            {"channel-1"},
            {currentEvent});

    assert(result.stored);
    assert(result.removedEvents.size() == 1);
    assert(result.removedEvents.front().eventId == "38845");
    assert(repository.containsEventForBackend(
        "default", "channel-1", "39568"));
    assert(!repository.containsEventForBackend(
        "default", "channel-1", "38845"));
    assert(repository.containsEventForBackend(
        "default", "channel-1", "outside"));
    assert(repository.containsEventForBackend(
        "default", "channel-2", "other"));
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_event_artwork WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_images WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_people WHERE event_id='38845';") == 0);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM suite_metadata_target_bindings "
        "WHERE metadata_target_id='target-old' AND lifecycle_state='retired';") == 1);
    assert(scalar_int(
        database,
        "SELECT COUNT(*) FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id='target-old' AND assignment_state='stale';") == 1);
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
    test_integer_window_uses_end_epoch_index();
    test_unchanged_upsert_does_not_rewrite_row();
    test_repository_is_backend_scoped();
    test_upsert_updates_only_matching_backend();
    test_authoritative_window_removes_only_missing_native_ids();
    test_window_and_cleanup_are_backend_scoped();

    return 0;
}
