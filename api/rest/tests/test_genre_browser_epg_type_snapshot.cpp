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
        "('default','C-1','1','Death in Paradise','Verletzter Stolz','',"
        "'1000','2000',1000,''),"
        "('default','C-1','2','The Big Bang Theory','Klozilla','',"
        "'1100','2100',1000,''),"
        "('default','C-1','3','Tagesschau','','','1200','2200',1000,''),"
        "('default','C-1','4','Tagesthemen','mit Wetter','',"
        "'1300','2300',1000,''),"
        "('default','C-1','5','Sportschau','Live','',"
        "'1400','2400',1000,'');"
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
    assert(repository.synchronizeEpgCache("default", 900, 2500));

    GenreBrowserApiRuntime& runtime = GenreBrowserApiRuntime::instance();
    assert(runtime.configure(database, backendRegistryService));

    const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem> items = {
        item("1", 1000, 2000),
        item("2", 1100, 2100),
        item("3", 1200, 2200),
        item("4", 1300, 2300),
        item("5", 1400, 2400),
    };
    assert(runtime.applyEpgTypeSnapshot("default", items));

    const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem> emptyPage;
    assert(runtime.applyEpgTypeSnapshot("default", emptyPage));

    ApiResponse series;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=series"
        "&from=900&until=2500",
        series));
    assert(series.statusCode == 200);
    assert(contains(series, "Death in Paradise"));
    assert(contains(series, "The Big Bang Theory"));
    assert(!contains(series, "Tagesschau"));
    assert(!contains(series, "Tagesthemen"));
    assert(!contains(series, "Sportschau"));

    ApiResponse sports;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=sports"
        "&from=900&until=2500",
        sports));
    assert(sports.statusCode == 200);
    assert(contains(sports, "Sportschau"));
    assert(!contains(sports, "Tagesschau"));

    runtime.reset();
    database.close();
    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());
    std::cout << "genre browser epg type snapshot ok\n";
    return 0;
}
