#include "Database.h"
#include "EpgArtworkRepository.h"

#include <cassert>
#include <cstdio>

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
}

int main()
{
    testArtworkIsPersistedAndBackendScoped();
    testArtworkUpsertAndRemoval();
    return 0;
}
