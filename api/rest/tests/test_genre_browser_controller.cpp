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
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,recording_key TEXT,backend_native_id TEXT,content_state TEXT,last_attempt_state TEXT,provider TEXT,PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_text_list(backend_id TEXT,recording_key TEXT,kind TEXT,ordinal INTEGER,value TEXT,PRIMARY KEY(backend_id,recording_key,kind,ordinal));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,duration_seconds INTEGER,content_descriptors TEXT,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"));
}

void seed(
    Database& database,
    std::int64_t now)
{
    const std::string sql =
        "INSERT INTO vdr_recording_cache VALUES"
        "('default','recording-1','recording-id-1','native/one','Space','Movies/Space','" +
        std::to_string(now - 7200) +
        "',5400,1000,''),"
        "('remote','recording-2','recording-id-2','native/two','Remote Space','Movies/Remote','" +
        std::to_string(now - 3600) +
        "',3600,700,'');"
        "INSERT INTO epg_events VALUES"
        "('default','C-1','100','Future Mystery','Episode','Description','" +
        std::to_string(now + 600) + "','" + std::to_string(now + 4200) +
        "',3600,'Mystery'),"
        "('remote','C-2','200','Remote Action','Episode','Description','" +
        std::to_string(now + 900) + "','" + std::to_string(now + 4500) +
        "',3600,'Action');"
        "INSERT INTO epg_event_artwork VALUES"
        "('default','C-1','100','tvscraper','/tmp/event.jpg',1280,720," +
        std::to_string(now) + ");";
    assert(database.execute(sql));
}

GenreEvidenceInput evidence(
    const std::string& backendId,
    const std::string& targetType,
    const std::string& resourceKey,
    const std::string& nativeId,
    const std::string& channelId,
    std::int64_t startTime,
    std::int64_t endTime,
    const std::string& genre)
{
    GenreEvidenceInput input;
    input.backendId = backendId;
    input.targetType = targetType;
    input.resourceKey = resourceKey;
    input.nativeId = nativeId;
    input.channelId = channelId;
    input.startTime = startTime;
    input.endTime = endTime;
    input.providerId = targetType == "recording" ? "recording-native" : "vdr-epg";
    input.sourceKind = targetType == "recording" ? "recording-metadata" : "dvb-content-descriptor";
    input.originalValues = {genre};
    input.confidence = 0.9;
    input.observedAt = startTime;
    return input;
}

bool contains(
    const ApiResponse& response,
    const std::string& text)
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
        resolution.attempted = true;
        resolution.found = true;
        resolution.metadata.backendId = backendId;
        resolution.metadata.channelId = event.channelId;
        resolution.metadata.eventId = event.id;
        resolution.metadata.provider = "tvscraper";
        resolution.metadata.mediaType = EpgScraperMediaType::Movie;
        resolution.metadata.title = event.title;
        resolution.metadata.genres = {"Sci-Fi", "Drama"};
        return resolution;
    }

    int calls = 0;
};
}

int main()
{
    const std::string filename = "/tmp/vdr-suite-genre-browser-controller-test.sqlite";
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
    assert(repository.replaceEvidence(evidence(
        "default", "recording", "recording-1", "native/one", "", 0, 0,
        "Science Fiction")));
    assert(repository.replaceEvidence(evidence(
        "remote", "recording", "recording-2", "native/two", "", 0, 0,
        "Science Fiction")));
    assert(repository.replaceEvidence(evidence(
        "default", "program-event", "C-1\n100", "100", "C-1",
        now + 600, now + 4200, "Mystery")));
    assert(repository.replaceEvidence(evidence(
        "remote", "program-event", "C-2\n200", "200", "C-2",
        now + 900, now + 4500, "Action")));

    GenreBrowserController controller(repository, backendRegistryService);

    const ApiResponse recordingOverview = controller.getOverview(
        "default", "recordings", "de", -1, -1);
    assert(recordingOverview.statusCode == 200);
    assert(contains(recordingOverview, "\"scope\":\"recordings\""));
    assert(contains(recordingOverview, "\"id\":\"science-fiction\""));
    assert(contains(recordingOverview, "\"count\":1"));

    const ApiResponse recordings = controller.getRecordings(
        "default", "science-fiction", 1, 0);
    assert(recordings.statusCode == 200);
    assert(contains(recordings, "\"title\":\"Space\""));
    assert(contains(recordings, "kind=preferred"));
    assert(!contains(recordings, "Remote Space"));

    const ApiResponse epgOverview = controller.getOverview(
        "default", "epg", "en", now, now + 172800);
    assert(epgOverview.statusCode == 200);
    assert(contains(epgOverview, "\"scope\":\"epg\""));
    assert(contains(epgOverview, "\"locale\":\"en\""));
    assert(contains(epgOverview, "\"id\":\"mystery\""));

    const ApiResponse epg = controller.getEpg(
        "default", "mystery", now, now + 172800, 1, 0);
    assert(epg.statusCode == 200);
    assert(contains(epg, "\"eventId\":\"100\""));
    assert(contains(epg, "\"available\":true"));
    assert(contains(epg, "/api/epg/cache/artwork?backend=default"));
    assert(!contains(epg, "Remote Action"));

    assert(controller.getOverview(
        "missing", "recordings", "de", -1, -1).statusCode == 404);
    assert(controller.getOverview(
        "default", "invalid", "de", -1, -1).statusCode == 400);
    assert(controller.getRecordings(
        "default", "does-not-exist", 10, 0).statusCode == 404);
    assert(controller.getEpg(
        "default", "", now, now + 100, 10, 0).statusCode == 400);

    GenreBrowserApiRuntime& runtime = GenreBrowserApiRuntime::instance();
    assert(runtime.configure(database, backendRegistryService));
    FakeEpgResolver resolver;
    runtime.registerEpgScraperMetadataResolver("default", resolver);

    ApiResponse routed;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres?backend=default&scope=epg&from=" +
            std::to_string(now) + "&until=" + std::to_string(now + 172800),
        routed));
    assert(routed.statusCode == 200);
    assert(resolver.calls == 0);
    assert(!runtime.tryHandleGet("/api/vdr/live/overlay", routed));

    assert(runtime.refreshEpgIndex(
        "default", now, now + 172800, 8));
    assert(resolver.calls == 1);

    ApiResponse enriched;
    assert(runtime.tryHandleGet(
        "/api/metadata/genres?backend=default&scope=epg&from=" +
            std::to_string(now) + "&until=" + std::to_string(now + 172800),
        enriched));
    assert(enriched.statusCode == 200);
    assert(contains(enriched, "\"id\":\"science-fiction\""));
    assert(resolver.calls == 1);

    runtime.reset();
    database.close();
    std::remove(filename.c_str());
    std::cout << "genre browser controller ok\n";
    return 0;
}
