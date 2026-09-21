#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "GenreBrowserApiRuntime.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool contains(const ApiResponse& response, const std::string& text)
{
    return response.body.find(text) != std::string::npos;
}

SuiteBridgeEpgTypeSnapshotTransportItem item(
    const std::string& eventId,
    std::int64_t startTime,
    std::int64_t endTime)
{
    SuiteBridgeEpgTypeSnapshotTransportItem value;
    value.channelId = "C-1";
    value.eventId = eventId;
    value.startTime = startTime;
    value.endTime = endTime;
    value.mediaType = EpgScraperMediaType::Series;
    return value;
}

GenreEvidenceInput scraperGenreEvidence(
    const std::string& eventId,
    std::int64_t startTime,
    std::int64_t endTime,
    const std::string& genre)
{
    GenreEvidenceInput value;
    value.backendId = "default";
    value.targetType = "program-event";
    value.resourceKey = "C-1\n" + eventId;
    value.nativeId = eventId;
    value.channelId = "C-1";
    value.startTime = startTime;
    value.endTime = endTime;
    value.providerId = "tvscraper";
    value.sourceKind = "scraper-metadata";
    value.originalValues = {genre};
    value.state = "active";
    value.confidence = 0.95;
    value.observedAt = 1500;
    return value;
}
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-browser-type-snapshot-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,"
        "recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,"
        "start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,"
        "metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,"
        "recording_key TEXT,backend_native_id TEXT,content_state TEXT,"
        "last_attempt_state TEXT,provider TEXT,"
        "PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_text_list(backend_id TEXT,"
        "recording_key TEXT,kind TEXT,ordinal INTEGER,value TEXT,"
        "PRIMARY KEY(backend_id,recording_key,kind,ordinal));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,"
        "event_id TEXT,title TEXT,subtitle TEXT,description TEXT,"
        "start_time TEXT,end_time TEXT,duration_seconds INTEGER,"
        "content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,"
        "event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,"
        "resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE vdr_channel_cache(backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,channel_number INTEGER NOT NULL DEFAULT 0,"
        "name TEXT NOT NULL DEFAULT '',provider TEXT NOT NULL DEFAULT '',"
        "group_name TEXT NOT NULL DEFAULT '',radio INTEGER NOT NULL DEFAULT 0,"
        "encrypted INTEGER NOT NULL DEFAULT 0,enabled INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(backend_id,channel_id));"
        "INSERT INTO epg_events VALUES"
        "('default','C-1','1','Criminal Intent - Verbrechen im Visier','Freiheitskampf','',"
        "'1000','2000',1000,''),"
        "('default','C-1','2','Velvet','Isabels Entscheidung','',"
        "'1100','2100',1000,''),"
        "('default','C-1','3','Tagesschau','','','1200','2200',1000,''),"
        "('default','C-1','4','Tagesthemen','mit Wetter','',"
        "'1300','2300',1000,''),"
        "('default','C-1','5','Sportschau','Live','',"
        "'1400','2400',1000,''),"
        "('default','C-1','6','Panda, Gorilla & Co.','Zoogeschichten','',"
        "'1500','2500',1000,''),"
        "('default','C-1','7','NFL Highlights','Spieltag','',"
        "'1600','2600',1000,''),"
        "('default','C-1','8','Nachrichten','','Aktuelle Meldungen',"
        "'1700','2700',1000,''),"
        "('default','C-1','9','NDR Talk Show','','',"
        "'1800','2800',1000,''),"
        "('default','C-1','10','Galileo','','Wissensmagazin',"
        "'1900','2900',1000,''),"
        "('default','C-1','11','SAT.1-Frühstücksfernsehen am Sonntag','','',"
        "'2000','3000',1000,''),"
        "('default','C-1','12','Kölner Treff','','',"
        "'2100','3100',1000,''),"
        "('default','C-1','13','Die Sendung mit der Maus','','',"
        "'2200','3200',1000,''),"
        "('default','C-1','14','Nordmagazin','','Regionalmagazin',"
        "'2300','3300',1000,'');"
        "INSERT INTO vdr_channel_cache(backend_id,channel_id,channel_number,name) "
        "VALUES('default','C-1',1,'Das Erste HD');"));

    BackendRegistry registry;
    BackendNode backend;
    backend.backendId = "default";
    backend.backendName = "Local";
    registry.addBackend(backend);
    BackendRegistryService backendRegistryService(registry);

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.synchronizeEpgCache("default", 900, 3400));

    GenreBrowserApiRuntime& runtime = GenreBrowserApiRuntime::instance();
    assert(runtime.configure(database, backendRegistryService));

    assert(repository.replaceEvidence(scraperGenreEvidence(
        "1", 1000, 2000, "Crime")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "2", 1100, 2100, "Drama")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "6", 1500, 2500, "Documentary")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "7", 1600, 2600, "Sports")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "9", 1800, 2800, "Talk Show")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "10", 1900, 2900, "Reality")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "11", 2000, 3000, "News")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "12", 2100, 3100, "Talk Show")));
    assert(repository.replaceEvidence(scraperGenreEvidence(
        "13", 2200, 3200, "Children")));

    const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem> items = {
        item("1", 1000, 2000),
        item("2", 1100, 2100),
        item("3", 1200, 2200),
        item("4", 1300, 2300),
        item("5", 1400, 2400),
        item("6", 1500, 2500),
        item("7", 1600, 2600),
        item("8", 1700, 2700),
        item("9", 1800, 2800),
        item("10", 1900, 2900),
        item("11", 2000, 3000),
        item("12", 2100, 3100),
        item("13", 2200, 3200),
        item("14", 2300, 3300),
    };
    assert(runtime.applyEpgTypeSnapshot("default", items));

    const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem> emptyPage;
    assert(runtime.applyEpgTypeSnapshot("default", emptyPage));

    ApiResponse series;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=series"
        "&from=900&until=3400",
        series));
    assert(series.statusCode == 200);
    assert(contains(series, "Criminal Intent - Verbrechen im Visier"));
    assert(contains(series, "Velvet"));
    assert(!contains(series, "Tagesschau"));
    assert(!contains(series, "Tagesthemen"));
    assert(!contains(series, "Sportschau"));
    assert(!contains(series, "Panda, Gorilla & Co."));
    assert(!contains(series, "NFL Highlights"));
    assert(!contains(series, "Nachrichten"));
    assert(!contains(series, "NDR Talk Show"));
    assert(!contains(series, "Galileo"));
    assert(!contains(series, "SAT.1-Frühstücksfernsehen am Sonntag"));
    assert(!contains(series, "Kölner Treff"));
    assert(!contains(series, "Die Sendung mit der Maus"));
    assert(!contains(series, "Nordmagazin"));

    ApiResponse documentary;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=documentary"
        "&from=900&until=3400",
        documentary));
    assert(documentary.statusCode == 200);
    assert(contains(documentary, "Panda, Gorilla & Co."));
    assert(!contains(documentary, "Nachrichten"));

    ApiResponse sports;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=sports"
        "&from=900&until=3400",
        sports));
    assert(sports.statusCode == 200);
    assert(contains(sports, "Sportschau"));
    assert(contains(sports, "NFL Highlights"));
    assert(!contains(sports, "Tagesschau"));
    assert(!contains(sports, "Nachrichten"));

    runtime.reset();
    database.close();
    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());
    std::cout << "genre browser epg type snapshot ok\n";
    return 0;
}
