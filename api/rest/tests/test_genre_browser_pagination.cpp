#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "GenreBrowserController.h"
#include "GenreIndexRepository.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
GenreEvidenceInput recordingEvidence(
    const std::string& resourceKey,
    const std::string& nativeId)
{
    GenreEvidenceInput input;
    input.backendId = "default";
    input.targetType = "recording";
    input.resourceKey = resourceKey;
    input.nativeId = nativeId;
    input.providerId = "recording-native";
    input.sourceKind = "recording-metadata";
    input.originalValues = {"Drama"};
    input.confidence = 0.9;
    input.observedAt = 1000;
    return input;
}

bool contains(
    const ApiResponse& response,
    const std::string& text)
{
    return response.body.find(text) != std::string::npos;
}
}

int main()
{
    const std::string filename = "/tmp/vdr-suite-genre-browser-pagination-test.sqlite";
    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,duration_seconds INTEGER,content_descriptors TEXT,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO vdr_recording_cache VALUES"
        "('default','r1','id1','native1','Older','Movies/Older','100',3600,500,''),"
        "('default','r2','id2','native2','Newer','Movies/Newer','200',3600,600,'');"));

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.replaceEvidence(recordingEvidence("r1", "native1")));
    assert(repository.replaceEvidence(recordingEvidence("r2", "native2")));

    BackendRegistry registry;
    BackendNode backend;
    backend.backendId = "default";
    registry.addBackend(backend);
    BackendRegistryService backendRegistryService(registry);
    GenreBrowserController controller(repository, backendRegistryService);

    const ApiResponse first = controller.getRecordings(
        "default", "drama", 1, 0);
    assert(first.statusCode == 200);
    assert(contains(first, "\"total\":2"));
    assert(contains(first, "\"limit\":1"));
    assert(contains(first, "\"offset\":0"));
    assert(contains(first, "\"hasMore\":true"));
    assert(contains(first, "\"title\":\"Newer\""));
    assert(!contains(first, "\"title\":\"Older\""));

    const ApiResponse second = controller.getRecordings(
        "default", "drama", 1, 1);
    assert(second.statusCode == 200);
    assert(contains(second, "\"offset\":1"));
    assert(contains(second, "\"hasMore\":false"));
    assert(contains(second, "\"title\":\"Older\""));

    const ApiResponse clampedWindow = controller.getOverview(
        "default",
        "epg",
        "de",
        1000,
        1000 + (30 * 24 * 60 * 60));
    assert(clampedWindow.statusCode == 200);
    assert(contains(clampedWindow, "\"from\":1000"));
    assert(contains(clampedWindow, "\"until\":605800"));

    database.close();
    std::remove(filename.c_str());
    std::cout << "genre browser pagination ok\n";
    return 0;
}
