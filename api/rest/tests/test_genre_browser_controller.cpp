#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "EpgScraperMetadata.h"
#include "GenreBrowserApiRuntime.h"
#include "GenreBrowserController.h"
#include "GenreIndexRepository.h"
#include "IEpgScraperMetadataResolver.h"

#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
std::int64_t nowEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void createSourceSchemas(Database& database)
{
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
        "CREATE TABLE epg_scraper_metadata_images("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,kind TEXT,"
        "image_index INTEGER,provider TEXT,path TEXT,width INTEGER,"
        "height INTEGER,resolved_at INTEGER,"
        "PRIMARY KEY(backend_id,channel_id,event_id,kind,image_index));"
        "CREATE TABLE vdr_channel_cache(backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,channel_number INTEGER NOT NULL DEFAULT 0,"
        "name TEXT NOT NULL DEFAULT '',provider TEXT NOT NULL DEFAULT '',"
        "group_name TEXT NOT NULL DEFAULT '',radio INTEGER NOT NULL DEFAULT 0,"
        "encrypted INTEGER NOT NULL DEFAULT 0,enabled INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(backend_id,channel_id));"));
}

void seed(Database& database, std::int64_t now)
{
    const std::string sql =
        "INSERT INTO vdr_recording_cache VALUES"
        "('default','recording-1','recording-id-1','native/one',"
        "'Space','Movies/Space','" + std::to_string(now - 7200) +
        "',5400,1000,''),"
        "('remote','recording-2','recording-id-2','native/two',"
        "'Remote Space','Movies/Remote','" + std::to_string(now - 3600) +
        "',3600,700,'');"
        "INSERT INTO epg_events VALUES"
        "('default','C-1','100','Future Thriller','Film','Description','" +
        std::to_string(now + 600) + "','" + std::to_string(now + 4200) +
        "',3600,'Film/Drama\nDetektiv/Thriller'),"
        "('default','C-1','101','Criminal Intent - Verbrechen im Visier','Episode','Description','" +
        std::to_string(now + 900) + "','" + std::to_string(now + 4500) +
        "',3600,'Film/Drama'),"
        "('default','C-1','102','News','Heute','Description','" +
        std::to_string(now + 1200) + "','" + std::to_string(now + 4800) +
        "',3600,'News'),"
        "('default','C-1','103','Nature Documentary','Natur','Description','" +
        std::to_string(now + 1500) + "','" + std::to_string(now + 5100) +
        "',3600,'Doku/Natur'),"
        "('default','C-1','104','Live Sport','Live','Description','" +
        std::to_string(now + 1800) + "','" + std::to_string(now + 5400) +
        "',3600,'Sport'),"
        "('default','C-1','105','Sportschau','Magazin','Description','" +
        std::to_string(now + 1850) + "','" + std::to_string(now + 5450) +
        "',3600,''),"
        "('default','C-1','106','Tagesschau','','Description','" +
        std::to_string(now + 1900) + "','" + std::to_string(now + 5500) +
        "',3600,''),"
        "('default','C-1','107','Tagesthemen','','Description','" +
        std::to_string(now + 1950) + "','" + std::to_string(now + 5550) +
        "',3600,''),"
        "('remote','C-2','200','Remote Action','Film','Description','" +
        std::to_string(now + 900) + "','" + std::to_string(now + 4500) +
        "',3600,'Film/Action');"
        "INSERT INTO epg_event_artwork VALUES"
        "('default','C-1','100','tvscraper','/tmp/event.jpg',1280,720," +
        std::to_string(now) + "),"
        "('default','C-1','101','tvscraper','/tmp/series-event.jpg',1280,720," +
        std::to_string(now) + ");"
        "INSERT INTO epg_scraper_metadata_images VALUES"
        "('default','C-1','100','preferred',0,'tvscraper',"
        "'/tmp/event-preferred.jpg',1280,720," + std::to_string(now) + "),"
        "('default','C-1','100','gallery',1,'tvscraper',"
        "'/tmp/event-poster.jpg',500,750," + std::to_string(now) + ");"
        "INSERT INTO vdr_channel_cache("
        "backend_id,channel_id,channel_number,name,provider,group_name,"
        "radio,encrypted,enabled) VALUES"
        "('default','C-1',1,'Das Erste HD','ARD',"
        "'Öffentlich-rechtlich',0,0,1),"
        "('remote','C-2',2,'Remote Channel','Remote','Remote',0,0,1);";
    assert(database.execute(sql));

    for (int index = 0; index < 62; ++index)
    {
        const std::string eventId = std::to_string(1000 + index);
        const std::string startTime = std::to_string(now + 2100 + index);
        const std::string endTime = std::to_string(now + 5700 + index);
        assert(database.execute(
            "INSERT INTO epg_events VALUES('default','C-1','" + eventId +
            "','Synthetic News " + eventId +
            "','Nachrichten','Description','" + startTime + "','" + endTime +
            "',3600,'News');"));
    }
}

GenreEvidenceInput recordingEvidence(
    const std::string& backendId,
    const std::string& resourceKey,
    const std::string& nativeId)
{
    GenreEvidenceInput input;
    input.backendId = backendId;
    input.targetType = "recording";
    input.resourceKey = resourceKey;
    input.nativeId = nativeId;
    input.providerId = "recording-native";
    input.sourceKind = "recording-metadata";
    input.originalValues = {"Science Fiction"};
    input.confidence = 0.9;
    input.observedAt = 1000;
    return input;
}

bool contains(const ApiResponse& response, const std::string& text)
{
    return response.body.find(text) != std::string::npos;
}

class FakeEpgResolver : public IEpgScraperMetadataResolver
{
public:
    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override
    {
        ++calls;
        EpgScraperMetadataResolution resolution;
        if (event.id == "102" || event.id == "104")
        {
            return resolution;
        }

        resolution.attempted = true;
        resolution.found = true;
        resolution.metadata.backendId = backendId;
        resolution.metadata.channelId = event.channelId;
        resolution.metadata.eventId = event.id;
        resolution.metadata.provider = "tvscraper";
        resolution.metadata.title = event.title;

        if (event.id == "101")
        {
            resolution.metadata.mediaType = EpgScraperMediaType::Series;
            resolution.metadata.genres = {"Crime"};
        }
        else if (event.id == "103")
        {
            resolution.metadata.mediaType = EpgScraperMediaType::Movie;
            resolution.metadata.genres = {"Documentary"};
        }
        else if (event.id == "105" || event.id == "106" || event.id == "107")
        {
            resolution.metadata.mediaType = EpgScraperMediaType::Series;
            resolution.metadata.genres = {"News"};
        }
        else
        {
            resolution.metadata.mediaType = EpgScraperMediaType::Movie;
            resolution.metadata.genres = {"Sci-Fi", "Thriller"};
        }
        return resolution;
    }

    int calls = 0;
};
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-browser-controller-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    createSourceSchemas(database);
    const std::int64_t now = nowEpochSeconds();
    seed(database, now);

    BackendRegistry registry;
    BackendNode local;
    local.backendId = "default";
    local.backendName = "Local";
    registry.addBackend(local);
    BackendNode remote;
    remote.backendId = "remote";
    remote.backendName = "Remote";
    remote.accessMode = "read-only";
    registry.addBackend(remote);
    BackendRegistryService backendRegistryService(registry);

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.replaceEvidence(recordingEvidence(
        "default", "recording-1", "native/one")));
    assert(repository.replaceEvidence(recordingEvidence(
        "remote", "recording-2", "native/two")));
    assert(repository.synchronizeEpgCache(
        "default", now, now + 172800));
    assert(repository.synchronizeEpgCache(
        "remote", now, now + 172800));

    GenreBrowserController controller(repository, backendRegistryService);

    const ApiResponse recordingOverview = controller.getOverview(
        "default", "recordings", "de", -1, -1);
    assert(recordingOverview.statusCode == 200);
    assert(contains(recordingOverview, "\"id\":\"science-fiction\""));

    const ApiResponse initialOverview = controller.getOverview(
        "default", "epg", "de", now, now + 172800);
    assert(initialOverview.statusCode == 200);
    assert(contains(initialOverview, "\"taxonomy\":\"epg-browse-v1\""));
    assert(contains(initialOverview, "\"id\":\"movie\""));
    assert(contains(initialOverview, "\"id\":\"series\""));
    assert(contains(initialOverview, "\"id\":\"documentary\""));
    assert(contains(initialOverview, "\"id\":\"sports\""));
    assert(!contains(initialOverview, "\"id\":\"news\""));

    const ApiResponse initialMovies = controller.getEpg(
        "default", "movie", "", now, now + 172800, 10, 0);
    assert(initialMovies.statusCode == 200);
    assert(contains(initialMovies, "\"eventId\":\"100\""));
    assert(!contains(initialMovies, "\"eventId\":\"101\""));
    assert(contains(
        initialMovies,
        "/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=100&kind=gallery&index=1"));
    assert(contains(initialMovies, "\"width\":500,\"height\":750"));

    GenreBrowserApiRuntime& runtime = GenreBrowserApiRuntime::instance();
    assert(runtime.configure(database, backendRegistryService));

    FakeEpgResolver resolver;
    runtime.registerEpgScraperMetadataResolver("default", resolver);

    ApiResponse routed;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres?backend=default&scope=epg&from=" +
            std::to_string(now) + "&until=" +
            std::to_string(now + 172800),
        routed));
    assert(routed.statusCode == 200);
    assert(resolver.calls == 0);
    assert(!runtime.tryHandleGet("/api/vdr/live/overlay", routed));

    assert(runtime.continueEpgEnrichment(
        "default", now, now + 172800, 70));
    assert(resolver.calls == 70);
    assert(runtime.continueEpgEnrichment(
        "default", now, now + 172800, 70));
    assert(resolver.calls == 70);

    ApiResponse series;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=series"
        "&from=" + std::to_string(now) +
        "&until=" + std::to_string(now + 172800),
        series));
    assert(series.statusCode == 200);
    assert(contains(series, "\"eventId\":\"101\""));
    assert(contains(series, "\"title\":\"Criminal Intent - Verbrechen im Visier\""));
    assert(contains(
        series,
        "/api/epg/cache/artwork?backend=default&channelId=C-1&eventId=101"));
    assert(contains(series, "\"width\":1280,\"height\":720"));
    assert(!contains(series, "Sportschau"));
    assert(!contains(series, "Tagesschau"));
    assert(!contains(series, "Tagesthemen"));

    ApiResponse sports;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=sports"
        "&from=" + std::to_string(now) +
        "&until=" + std::to_string(now + 172800),
        sports));
    assert(sports.statusCode == 200);
    assert(contains(sports, "Sportschau"));
    assert(!contains(sports, "Tagesschau"));
    assert(!contains(sports, "Tagesthemen"));

    ApiResponse movies;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=movie"
        "&from=" + std::to_string(now) +
        "&until=" + std::to_string(now + 172800),
        movies));
    assert(movies.statusCode == 200);
    assert(contains(movies, "\"eventId\":\"100\""));
    assert(!contains(movies, "\"eventId\":\"101\""));
    assert(contains(movies, "\"channelName\":\"Das Erste HD\""));
    assert(contains(movies, "\"available\":true"));
    assert(contains(
        movies,
        "/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=100&kind=gallery&index=1"));

    ApiResponse thriller;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres/epg?backend=default&contentClass=movie"
        "&genre=thriller&from=" + std::to_string(now) +
        "&until=" + std::to_string(now + 172800),
        thriller));
    assert(thriller.statusCode == 200);
    assert(contains(thriller, "\"eventId\":\"100\""));
    assert(contains(
        thriller,
        "/api/epg/cache/metadata/image?backend=default&channelId=C-1&eventId=100&kind=gallery&index=1"));

    runtime.reset();
    database.close();
    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());
    std::cout << "genre browser controller hierarchy ok\n";
    return 0;
}
