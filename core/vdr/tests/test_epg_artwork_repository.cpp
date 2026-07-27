#include "Database.h"
#include "EpgArtworkRepository.h"

#include <cassert>
#include <cstdio>
#include <sqlite3.h>

namespace
{
EpgArtworkReference makeArtwork(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId,
    const std::string& path)
{
    EpgArtworkReference artwork;
    artwork.backendId = backendId;
    artwork.channelId = channelId;
    artwork.eventId = eventId;
    artwork.provider = "tvscraper";
    artwork.path = path;
    artwork.width = 1280;
    artwork.height = 720;
    artwork.resolvedAt = 123456789;
    return artwork;
}


int countRows(Database& database, const char* sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr) == SQLITE_OK);
    int count = 0;
    if (sqlite3_step(statement) == SQLITE_ROW) count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return count;
}

void testMetadataPeopleAreNormalizedReplacedAndEventGuarded()
{
    const char* databasePath = "/tmp/vdr-suite-epg-artwork-people-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES('default','channel-1','event-1');"));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());

    EpgScraperPerson actor;
    actor.role = EpgScraperPersonRole::Actor;
    actor.name = "JÖHN TrAVÖLTA";
    actor.characterName = "Vincent Vega";
    EpgScraperPerson director;
    director.role = EpgScraperPersonRole::Director;
    director.name = "Quentin Tarantino";

    assert(repository.replaceMetadataPeople(
        "default", "channel-1", "event-1", {actor, director}));
    assert(countRows(database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_people "
        "WHERE backend_id='default' AND channel_id='channel-1' AND event_id='event-1';") == 2);
    assert(countRows(database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_people "
        "WHERE name_folded='joehn travoelta' AND role='actor';") == 1);

    assert(repository.replaceMetadataPeople(
        "default", "channel-1", "event-1", {director}));
    assert(countRows(database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_people "
        "WHERE backend_id='default' AND channel_id='channel-1' AND event_id='event-1';") == 1);
    assert(!repository.replaceMetadataPeople(
        "default", "channel-1", "retired-event", {actor}));
}

void testArtworkIsPersistedAndBackendScoped()
{
    const char* databasePath = "/tmp/vdr-suite-epg-artwork-repository-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("epg_event_artwork"));

    assert(repository.upsert(makeArtwork(
        "home-vdr",
        "S19.2E-1-1019-10301",
        "4711",
        "/var/cache/vdr/tvscraper/home.jpg")));
    assert(repository.upsert(makeArtwork(
        "remote-vdr",
        "S19.2E-1-1019-10301",
        "4711",
        "/var/cache/vdr/tvscraper/remote.jpg")));

    const EpgArtworkReference home = repository.find(
        "home-vdr",
        "S19.2E-1-1019-10301",
        "4711");
    const EpgArtworkReference remote = repository.find(
        "remote-vdr",
        "S19.2E-1-1019-10301",
        "4711");

    assert(home.valid());
    assert(home.path == "/var/cache/vdr/tvscraper/home.jpg");
    assert(home.width == 1280);
    assert(home.height == 720);
    assert(home.resolvedAt == 123456789);

    assert(remote.valid());
    assert(remote.path == "/var/cache/vdr/tvscraper/remote.jpg");
}

void testArtworkUpsertAndRemoval()
{
    const char* databasePath = "/tmp/vdr-suite-epg-artwork-repository-update-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());

    EpgArtworkReference artwork = makeArtwork(
        "home-vdr",
        "channel-1",
        "event-1",
        "/old.jpg");
    assert(repository.upsert(artwork));

    artwork.path = "/new.jpg";
    artwork.width = 1920;
    artwork.height = 1080;
    artwork.resolvedAt = 987654321;
    assert(repository.upsert(artwork));

    const EpgArtworkReference updated = repository.find(
        "home-vdr",
        "channel-1",
        "event-1");
    assert(updated.path == "/new.jpg");
    assert(updated.width == 1920);
    assert(updated.height == 1080);
    assert(updated.resolvedAt == 987654321);

    assert(repository.removeForEvent(
        "home-vdr",
        "channel-1",
        "event-1"));
    assert(!repository.find(
        "home-vdr",
        "channel-1",
        "event-1").valid());
}

void testArtworkRejectsRetiredEventWhenEventCacheExists()
{
    const char* databasePath =
        "/tmp/vdr-suite-epg-artwork-repository-event-guard-test.db";
    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES('default','channel-1','current');"));

    EpgArtworkReference current = makeArtwork(
        "default", "channel-1", "current", "/current.jpg");
    EpgArtworkReference stale = makeArtwork(
        "default", "channel-1", "stale", "/stale.jpg");

    assert(repository.upsert(current));
    assert(!repository.upsert(stale));
    assert(repository.upsertMetadataJson(
        "default", "channel-1", "current", "{}", 1));
    assert(!repository.upsertMetadataJson(
        "default", "channel-1", "stale", "{}", 1));
    assert(repository.upsertMetadataImage(
        "default", "channel-1", "current", "preferred", 0, current));
    assert(!repository.upsertMetadataImage(
        "default", "channel-1", "stale", "preferred", 0, stale));
}

}

int main()
{
    testArtworkIsPersistedAndBackendScoped();
    testArtworkUpsertAndRemoval();
    testArtworkRejectsRetiredEventWhenEventCacheExists();
    testMetadataPeopleAreNormalizedReplacedAndEventGuarded();
    return 0;
}
