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
    artwork.origin = EpgArtworkReferenceOrigin::ExternalFallback;
    artwork.path = "/var/cache/vdr-suite/epg-artwork/external/series.png";
    artwork.width = 640;
    artwork.height = 360;
    artwork.resolvedAt = 123;
    assert(repository.upsert(artwork));
    assert(repository.referenceStateForPath(artwork.path) ==
           EpgSeriesArtworkFallbackPathReferenceState::Referenced);
    assert(repository.referenceStateForPath(
               "/var/cache/vdr-suite/epg-artwork/external/missing.png") ==
           EpgSeriesArtworkFallbackPathReferenceState::Unreferenced);
    assert(repository.referenceStateForPath("relative/series.png") ==
           EpgSeriesArtworkFallbackPathReferenceState::Error);

    EpgArtworkReference found = repository.find(
        "backend",
        "channel",
        "event");
    assert(found.valid());
    assert(found.provider == "provider");
    assert(found.origin == EpgArtworkReferenceOrigin::ExternalFallback);
    assert(found.path == artwork.path);
    assert(found.width == 640);
    assert(found.height == 360);
    assert(found.resolvedAt == 123);

    const std::string previousPath = artwork.path;
    artwork.path = "/var/cache/vdr-suite/epg-artwork/external/new.jpg";
    artwork.width = 1280;
    artwork.height = 720;
    artwork.resolvedAt = 456;
    assert(repository.upsert(artwork));
    assert(repository.referenceStateForPath(previousPath) ==
           EpgSeriesArtworkFallbackPathReferenceState::Unreferenced);
    assert(repository.referenceStateForPath(artwork.path) ==
           EpgSeriesArtworkFallbackPathReferenceState::Referenced);
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

    EpgArtworkReference unknownOrigin = artwork;
    unknownOrigin.origin = EpgArtworkReferenceOrigin::Unknown;
    assert(!repository.upsert(unknownOrigin));

    EpgArtworkReference invalidProvider = artwork;
    invalidProvider.provider = "TMDB bearer";
    assert(!repository.upsert(invalidProvider));

    EpgArtworkReference relativePath = artwork;
    relativePath.path = "external/series.png";
    assert(!repository.upsert(relativePath));

    EpgArtworkReference unresolved = artwork;
    unresolved.resolvedAt = 0;
    assert(!repository.upsert(unresolved));

    assert(repository.removeForEvent("backend", "channel", "event"));
    assert(repository.referenceStateForPath(artwork.path) ==
           EpgSeriesArtworkFallbackPathReferenceState::Unreferenced);
    assert(!repository.find("backend", "channel", "event").valid());

    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT);"));
    assert(!repository.upsert(artwork));
    assert(database.execute(
        "INSERT INTO epg_events VALUES('backend','channel','event');"));
    assert(repository.upsert(artwork));

    Database legacyDatabase;
    assert(legacyDatabase.open(":memory:"));
    assert(legacyDatabase.execute(
        "CREATE TABLE epg_series_artwork_fallback ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "provider TEXT NOT NULL,"
        "path TEXT NOT NULL,"
        "width INTEGER NOT NULL,"
        "height INTEGER NOT NULL,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, channel_id, event_id));"
        "INSERT INTO epg_series_artwork_fallback "
        "(backend_id,channel_id,event_id,provider,path,width,height,resolved_at) "
        "VALUES ('legacy','channel','event','tmdb',"
        "'/var/cache/vdr-suite/epg-artwork/external/legacy.jpg',"
        "1280,720,999);"));

    EpgSeriesArtworkFallbackRepository legacyRepository(legacyDatabase);
    assert(legacyRepository.ensureSchema());
    const EpgArtworkReference migrated = legacyRepository.find(
        "legacy",
        "channel",
        "event");
    assert(migrated.valid());
    assert(migrated.origin == EpgArtworkReferenceOrigin::ExternalFallback);
    assert(migrated.provider == "tmdb");
    assert(legacyRepository.referenceStateForPath(migrated.path) ==
           EpgSeriesArtworkFallbackPathReferenceState::Referenced);

    return 0;
}
