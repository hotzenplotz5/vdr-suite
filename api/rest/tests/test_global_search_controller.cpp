#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "GlobalSearchApiRuntime.h"
#include "GlobalSearchController.h"
#include "GlobalSearchRepository.h"
#include "GlobalSearchPersonPortraitRepository.h"
#include "GlobalSearchService.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
void createSchemas(Database& database)
{
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,recording_key TEXT,backend_native_id TEXT,content_state TEXT,title TEXT,original_title TEXT,episode_name TEXT,preferred_artwork_path TEXT,PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_person(backend_id TEXT,recording_key TEXT,ordinal INTEGER,role TEXT,name TEXT,name_folded TEXT,normalized_name TEXT,character_name TEXT,character_name_folded TEXT,PRIMARY KEY(backend_id,recording_key,ordinal));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,duration_seconds INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_cache(backend_id TEXT,channel_id TEXT,event_id TEXT,public_json TEXT,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO vdr_recording_cache VALUES('default','r1','id-1','native-1','Pulp Fiction','Filme/Pulp','1785000000',9000,2048,'');"
        "INSERT INTO vdr_recording_native_metadata VALUES('default','rk1','native-1','found','Pulp Fiction','Pulp Fiction','','/var/cache/vdr-suite/recording-metadata/posters/pulp.jpg');"
        "INSERT INTO vdr_recording_native_person VALUES('default','rk1',0,'actor','John Travolta','john travolta','john travolta','','');"
        "INSERT INTO epg_events VALUES('default','c1','e1','Pulp Fiction','','','1785100000','1785109000',9000);"
        "INSERT INTO epg_scraper_metadata_cache VALUES('default','c1','e1','{\"available\":true,\"people\":[{\"role\":\"actor\",\"name\":\"John Travolta\",\"characterName\":\"\"}]}',1780000000);"));
}


void verifyPortraitIndices()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,backend_native_id TEXT);"
        "CREATE TABLE suite_metadata_manual_assignment_values(metadata_assignment_id TEXT,backend_id TEXT,resource_key TEXT,revision INTEGER);"
        "CREATE TABLE suite_metadata_assignments(metadata_assignment_id TEXT,assignment_state TEXT,manual_assignment INTEGER,relationship_locked INTEGER);"
        "CREATE TABLE suite_metadata_recording_person_relations(metadata_assignment_id TEXT,person_entity_id TEXT,role TEXT,ordinal INTEGER);"
        "CREATE TABLE suite_metadata_person_values(metadata_entity_id TEXT,display_name TEXT,provider_id TEXT,external_namespace TEXT,external_id TEXT,name_folded TEXT);"
        "CREATE TABLE suite_metadata_person_profiles(provider_id TEXT,external_namespace TEXT,external_id TEXT,local_path TEXT);"
        "INSERT INTO vdr_recording_cache VALUES('default','key','native-1'),('other','key','native-other');"
        "INSERT INTO suite_metadata_manual_assignment_values VALUES('a','default','key',7),('b','other','key',9);"
        "INSERT INTO suite_metadata_assignments VALUES('a','selected',1,1),('b','selected',1,1);"
        "INSERT INTO suite_metadata_recording_person_relations VALUES"
        "('a','p0','actor',0),('a','p1','actor',1),('a','p2','actor',2),('a','p3','actor',3),('b','p3','actor',0);"
        "INSERT INTO suite_metadata_person_values VALUES"
        "('p0','No Profile','tmdb','person','0','no profile'),"
        "('p1','First Portrait','tmdb','person','1','first portrait'),"
        "('p2','Empty Profile','tmdb','person','2','empty profile'),"
        "('p3','Second Portrait','tmdb','person','3','second portrait');"
        "INSERT INTO suite_metadata_person_profiles VALUES"
        "('tmdb','person','1','/cache/first.jpg'),('tmdb','person','2',''),('tmdb','person','3','/cache/second.jpg');"));
    GlobalSearchPersonPortraitRepository repository(database);
    const auto found = repository.findForBackend("default");
    assert(found.size() == 2U);
    assert(found[0].name == "First Portrait" && found[0].index == 1);
    assert(found[1].name == "Second Portrait" && found[1].index == 3);
    for (const auto& portrait : found)
    {
        assert(portrait.backendNativeId == "native-1");
        assert(portrait.assignmentRevision == 7);
    }
    const auto other = repository.findForBackend("other");
    assert(other.size() == 1U && other[0].index == 0);
    assert(other[0].backendNativeId == "native-other");
    assert(repository.findForBackend("missing").empty());
}

BackendNode backend(const std::string& id, bool enabled)
{
    BackendNode value;
    value.backendId = id;
    value.backendName = id;
    value.backendType = "restfulapi";
    value.enabled = enabled;
    value.online = true;
    return value;
}

std::vector<GlobalSearchPersonPortrait> portraits(
    const std::string& backendId)
{
    if (backendId != "default") return {};
    GlobalSearchPersonPortrait portrait;
    portrait.name = "John Travolta";
    portrait.role = "actor";
    portrait.backendNativeId = "native-1";
    portrait.index = 0;
    portrait.assignmentRevision = 7;
    return {portrait};
}
}

int main()
{
    verifyPortraitIndices();
    const char* databasePath = "/tmp/vdr-suite-global-search-controller-test.db";
    std::remove(databasePath);
    Database database;
    assert(database.open(databasePath));
    createSchemas(database);
    GlobalSearchRepository repository(database);
    assert(repository.ensureSchema());
    GlobalSearchService service(repository);

    BackendRegistry registry;
    registry.addBackend(backend("default", true));
    registry.addBackend(backend("disabled", false));
    BackendRegistryService registryService(registry);
    GlobalSearchController controller(service, registryService, portraits);

    ApiResponse response = controller.search("default", "", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"status\":\"empty\"") != std::string::npos);
    response = controller.search("default", "   ", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"status\":\"empty\"") != std::string::npos);

    response = controller.search("default", "J", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"status\":\"too-short\"") != std::string::npos);
    response = controller.search("default", "é", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"status\":\"too-short\"") != std::string::npos);

    response = controller.search("default", "John Travolta", 1785000000, 1785400000, 1, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"backendId\":\"default\"") != std::string::npos);
    assert(response.body.find("\"recordingTotal\":1") != std::string::npos);
    assert(response.body.find("\"epgTotal\":1") != std::string::npos);
    assert(response.body.find("\"matchedPerson\":\"John Travolta\"") != std::string::npos);
    const std::string recordingArtworkUrl =
        "/api/recordings/metadata/image?backend=default&backendNativeId=native-1&kind=preferred&index=0";
    assert(response.body.find(
        "\"artwork\":{\"available\":true,\"url\":\"" +
        recordingArtworkUrl + "\"}") != std::string::npos);
    assert(response.body.find(
        "\"metadata\":{\"presentation\":{\"posterUrl\":\"" +
        recordingArtworkUrl +
        "\"},\"artwork\":{\"preferredUrl\":\"" +
        recordingArtworkUrl + "\"}}") != std::string::npos);
    const std::string portraitUrl =
        "/api/recordings/metadata/image?backend=default&backendNativeId=native-1&kind=person&index=0&assignmentRevision=7";
    assert(response.body.find(
        "\"image\":{\"available\":true,\"url\":\"" +
        portraitUrl + "\"}") != std::string::npos);
    assert(response.body.find("/var/cache/") == std::string::npos);
    assert(response.body.find("image.tmdb.org") == std::string::npos);

    response = controller.search("default", "Kein Treffer", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 200);
    assert(response.body.find("\"recordingTotal\":0") != std::string::npos);
    assert(response.body.find("\"epgTotal\":0") != std::string::npos);

    response = controller.search("missing", "Pulp", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 404);
    response = controller.search("disabled", "Pulp", 1785000000, 1785400000, 20, 0);
    assert(response.statusCode == 403);
    response = controller.search("default", "Pulp", 1785000000, 1785400000, -1, 0);
    assert(response.statusCode == 400);

    GlobalSearchApiRuntime& runtime = GlobalSearchApiRuntime::instance();
    runtime.reset();
    runtime.setPersonPortraitLookup(portraits);
    int identityLookups = 0;
    assert(runtime.configure(database, registryService,
        [&identityLookups](const std::string& backend, const std::string& native)
        {
            ++identityLookups;
            assert(backend == "default");
            assert(native == "native-1");
            return std::string("rec_0123456789abcdef0123456789abcdef");
        }));
    ApiResponse routed;
    assert(runtime.tryHandleGet(
        "/api/search?backend=default&query=John%20Travolta&from=1785000000&until=1785400000&limit=1&offset=0",
        routed));
    assert(routed.statusCode == 200);
    assert(routed.body.find("\"recordingTotal\":1") != std::string::npos);
    assert(routed.body.find("\"epgTotal\":1") != std::string::npos);
    assert(routed.body.find(portraitUrl) != std::string::npos);
    assert(identityLookups == 1);
    assert(routed.body.find("\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\"") != std::string::npos);
    assert(routed.body.find("\"backendNativeId\":\"native-1\"") != std::string::npos);
    assert(runtime.tryHandleGet(
        "/api/search?backend=default&query=Kein%20Treffer&from=1785000000&until=1785400000",
        routed));
    assert(routed.statusCode == 200);
    assert(identityLookups == 1);
    assert(routed.body.find("publicRecordingId") == std::string::npos);
    assert(!runtime.tryHandleGet("/api/other", routed));
    runtime.reset();

    std::puts("global search controller tests passed");
    return 0;
}