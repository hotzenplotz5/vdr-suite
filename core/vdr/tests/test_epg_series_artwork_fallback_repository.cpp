#include "Database.h"
#include "EpgSeriesArtworkFallbackRepository.h"

#include <cassert>

int main()
{
    Database database;
    assert(database.open(":memory:"));
    EpgSeriesArtworkFallbackRepository repository(database);
    assert(repository.ensureSchema());
    assert(database.tableExists("epg_series_artwork_fallback"));

    EpgArtworkReference artwork;
    artwork.backendId = "backend";
    artwork.channelId = "channel";
    artwork.eventId = "event";
    artwork.provider = "provider";
    artwork.path = "/var/cache/vdr-suite/epg-artwork/external/series.png";
    artwork.width = 640;
    artwork.height = 360;
    artwork.resolvedAt = 123;
    assert(repository.upsert(artwork));

    EpgArtworkReference found = repository.find(
        "backend",
        "channel",
        "event");
    assert(found.valid());
    assert(found.provider == "provider");
    assert(found.path == artwork.path);
    assert(found.width == 640);
    assert(found.height == 360);
    assert(found.resolvedAt == 123);

    artwork.path = "/var/cache/vdr-suite/epg-artwork/external/new.jpg";
    artwork.width = 1280;
    artwork.height = 720;
    artwork.resolvedAt = 456;
    assert(repository.upsert(artwork));
    found = repository.find("backend", "channel", "event");
    assert(found.path == artwork.path);
    assert(found.width == 1280);
    assert(found.resolvedAt == 456);

    EpgArtworkReference primary = artwork;
    primary.provider = "tvscraper";
    assert(!repository.upsert(primary));

    EpgArtworkReference none = artwork;
    none.provider = "none";
    assert(!repository.upsert(none));

    assert(repository.removeForEvent("backend", "channel", "event"));
    assert(!repository.find("backend", "channel", "event").valid());

    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT);"));
    assert(!repository.upsert(artwork));
    assert(database.execute(
        "INSERT INTO epg_events VALUES('backend','channel','event');"));
    assert(repository.upsert(artwork));

    return 0;
}
