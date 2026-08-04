#include "Database.h"
#include "GlobalSearchRepository.h"
#include "VdrRecordingMetadataCacheCodec.h"

#include <sqlite3.h>

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
void execute(Database& database, const std::string& sql)
{
    assert(database.execute(sql));
}

void createExistingSchemas(Database& database)
{
    execute(database,
        "CREATE TABLE vdr_recording_cache(backend_id TEXT,cache_key TEXT,recording_id TEXT,backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata(backend_id TEXT,recording_key TEXT,backend_native_id TEXT,content_state TEXT,title TEXT,original_title TEXT,episode_name TEXT,preferred_artwork_path TEXT,PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_person(backend_id TEXT,recording_key TEXT,ordinal INTEGER,role TEXT,name TEXT,name_folded TEXT,normalized_name TEXT,character_name TEXT,character_name_folded TEXT,PRIMARY KEY(backend_id,recording_key,ordinal));"
        "CREATE TABLE epg_events(backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,duration_seconds INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_scraper_metadata_cache(backend_id TEXT,channel_id TEXT,event_id TEXT,public_json TEXT,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE epg_event_artwork(backend_id TEXT,channel_id TEXT,event_id TEXT,provider TEXT,path TEXT,width INTEGER,height INTEGER,resolved_at INTEGER,PRIMARY KEY(backend_id,channel_id,event_id));"
        "CREATE TABLE vdr_channel_cache(backend_id TEXT,channel_id TEXT,name TEXT,PRIMARY KEY(backend_id,channel_id));"
        "CREATE TABLE suite_metadata_assignments(metadata_assignment_id TEXT PRIMARY KEY,assignment_state TEXT,manual_assignment INTEGER,relationship_locked INTEGER);"
        "CREATE TABLE suite_metadata_manual_assignment_values(metadata_assignment_id TEXT PRIMARY KEY,backend_id TEXT,resource_key TEXT,media_type TEXT,title TEXT,original_title TEXT,poster_reference TEXT);"
        "CREATE TABLE suite_metadata_person_values(metadata_entity_id TEXT PRIMARY KEY,display_name TEXT,name_folded TEXT,external_id TEXT);"
        "CREATE TABLE suite_metadata_recording_person_relations(metadata_assignment_id TEXT,person_entity_id TEXT,role TEXT,ordinal INTEGER);");
}

std::string recordingPayload(const std::string& subtitle)
{
    VdrRecordingMetadata metadata;
    metadata.native.shortText = subtitle;
    return VdrRecordingMetadataCacheCodec::encode(metadata);
}

void insertFixtures(Database& database)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(database.handle(),
        "INSERT INTO vdr_recording_cache VALUES(?,?,?,?,?,?,?,?,?,?);",
        -1, &statement, nullptr) == SQLITE_OK);
    const auto insertRecording = [&](const char* backend, const char* key, const char* id,
                                     const char* nativeId, const char* title, const char* path,
                                     const char* start, const char* subtitle) {
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        const char* values[] = {backend,key,id,nativeId,title,path,start};
        for (int index = 0; index < 7; ++index)
            sqlite3_bind_text(statement,index+1,values[index],-1,SQLITE_TRANSIENT);
        sqlite3_bind_int(statement,8,9000);
        sqlite3_bind_int64(statement,9,2048);
        const std::string payload = recordingPayload(subtitle);
        sqlite3_bind_text(statement,10,payload.c_str(),-1,SQLITE_TRANSIENT);
        assert(sqlite3_step(statement) == SQLITE_DONE);
    };
    insertRecording("default","r1","recording-1","native-1","Pulp Fiction","Filme/Pulp Fiction","1785000000","Director's Cut");
    insertRecording("default","r2","recording-2","native-2","München","Filme/München","1784000000","Eine Stadtgeschichte");
    insertRecording("second","r3","recording-3","native-3","Pulp Fiction","Andere/Pulp Fiction","1786000000","Nicht im Default-Backend");
    sqlite3_finalize(statement);

    execute(database,
        "INSERT INTO vdr_recording_native_metadata VALUES('default','rk1','native-1','found','Pulp Fiction','Pulp Fiction','', '/poster/pulp.jpg');"
        "INSERT INTO vdr_recording_native_metadata VALUES('default','rk2','native-2','found','München','Munich','Folge München','');"
        "INSERT INTO vdr_recording_native_metadata VALUES('second','rk3','native-3','found','Pulp Fiction','Pulp Fiction','','');"
        "INSERT INTO vdr_recording_native_person VALUES('default','rk1',0,'actor','John Travolta','john travolta','john travolta','Vincent Vega','vincent vega');"
        "INSERT INTO vdr_recording_native_person VALUES('default','rk1',1,'actor','Uma Thurman','uma thurman','uma thurman','Mia Wallace','mia wallace');"
        "INSERT INTO vdr_recording_native_person VALUES('second','rk3',0,'actor','John Travolta','john travolta','john travolta','Vincent Vega','vincent vega');"
        "INSERT INTO epg_events VALUES('default','channel-1','event-1','Pulp Fiction','Spielfilm','Kultfilm','1785100000','1785109000',9000);"
        "INSERT INTO epg_events VALUES('default','channel-2','event-2','Saturday Night Fever','Tanzfilm','Mit John Travolta','1785200000','1785207200',7200);"
        "INSERT INTO epg_events VALUES('default','channel-3','event-3','München heute','Magazin','Lokales','1785300000','1785303600',3600);"
        "INSERT INTO epg_events VALUES('second','channel-4','event-4','Pulp Fiction','Film','Anderes Backend','1785100000','1785109000',9000);"
        "INSERT INTO vdr_channel_cache VALUES('default','channel-1','Arte');"
        "INSERT INTO vdr_channel_cache VALUES('default','channel-2','ZDFneo');"
        "INSERT INTO vdr_channel_cache VALUES('default','channel-3','BR');"
        "INSERT INTO epg_event_artwork VALUES('default','channel-1','event-1','tvscraper','/poster/epg.jpg',600,900,1780000000);"
        "INSERT INTO epg_scraper_metadata_cache VALUES('default','channel-1','event-1','{\"available\":true,\"title\":\"Pulp Fiction\",\"people\":[{\"role\":\"actor\",\"name\":\"John Travolta\",\"characterName\":\"Vincent Vega\"}]}',1780000000);"
        "INSERT INTO epg_scraper_metadata_cache VALUES('default','channel-2','event-2','{\"available\":true,\"title\":\"Saturday Night Fever\",\"people\":[{\"role\":\"actor\",\"name\":\"John Travolta\",\"characterName\":\"Tony Manero\"}]}',1780000000);"
        "INSERT INTO epg_scraper_metadata_cache VALUES('default','channel-3','event-3','{\"available\":true,\"title\":\"München heute\",\"people\":[]}',1780000000);"
        "INSERT INTO epg_scraper_metadata_cache VALUES('second','channel-4','event-4','{\"available\":true,\"title\":\"Pulp Fiction\",\"people\":[{\"role\":\"actor\",\"name\":\"John Travolta\",\"characterName\":\"Vincent Vega\"}]}',1780000000);"
        "INSERT INTO suite_metadata_assignments VALUES('manual-1','selected',1,1);"
        "INSERT INTO suite_metadata_manual_assignment_values VALUES('manual-1','default','r1','movie','Forrest Gump','The Forrest Original','');"
        "INSERT INTO suite_metadata_person_values VALUES('person-31','Tom Hanks','tom hanks','31');"
        "INSERT INTO suite_metadata_recording_person_relations VALUES('manual-1','person-31','actor',0);");
}

struct TraceState
{
    int manualSearchReads = 0;
    int schemaStatements = 0;
};

int traceSql(unsigned int kind, void* context, void* statement, void*)
{
    if (kind != SQLITE_TRACE_STMT || context == nullptr || statement == nullptr)
        return 0;
    const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
    if (sql == nullptr) return 0;
    TraceState& state = *static_cast<TraceState*>(context);
    if (std::strstr(sql, "WITH active_manual AS")) ++state.manualSearchReads;
    if (std::strstr(sql, "CREATE TABLE")) ++state.schemaStatements;
    return 0;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    createExistingSchemas(database);
    insertFixtures(database);

    GlobalSearchRepository repository(database);
    assert(repository.ensureSchema());
    const int writesAfterFirstSchema = sqlite3_total_changes(database.handle());
    assert(repository.ensureSchema());
    assert(sqlite3_total_changes(database.handle()) == writesAfterFirstSchema);
    assert(repository.ready());
    assert(GlobalSearchRepository::foldText("MÜNCHEN") == "muenchen");

    const int writesBefore = sqlite3_total_changes(database.handle());
    const GlobalSearchResult legacyTitle = repository.search(
        "default", "Pulp Fiction", 1785000000, 1785400000, 20, 0);
    const int writesAfter = sqlite3_total_changes(database.handle());
    assert(writesBefore == writesAfter);
    assert(legacyTitle.recordingTotal == 1);
    assert(legacyTitle.epgTotal == 1);
    assert(legacyTitle.recordings[0].title == "Forrest Gump");
    assert(legacyTitle.epg[0].channelName == "Arte");

    TraceState traceState;
    assert(sqlite3_trace_v2(
        database.handle(), SQLITE_TRACE_STMT, traceSql, &traceState) == SQLITE_OK);
    const GlobalSearchResult manualTitle = repository.search(
        "default", "Forrest Gump", 1785000000, 1785400000, 20, 0);
    assert(manualTitle.recordingTotal == 1);
    assert(manualTitle.recordings[0].title == "Forrest Gump");
    assert(manualTitle.recordings[0].matchReason == "title");
    assert(traceState.manualSearchReads == 2);
    assert(traceState.schemaStatements == 0);
    sqlite3_trace_v2(database.handle(), 0, nullptr, nullptr);

    const GlobalSearchResult manualOriginal = repository.search(
        "default", "The Forrest Original", 1785000000, 1785400000, 20, 0);
    assert(manualOriginal.recordingTotal == 1);
    assert(manualOriginal.recordings[0].title == "Forrest Gump");

    const GlobalSearchResult manualPerson = repository.search(
        "default", "Tom Hanks", 1785000000, 1785400000, 20, 0);
    assert(manualPerson.recordingTotal == 1);
    assert(manualPerson.recordings[0].matchedPerson == "Tom Hanks");
    assert(manualPerson.recordings[0].matchReason == "person");
    assert(manualPerson.people.size() == 1U);
    assert(manualPerson.people[0].recordingCount == 1);

    const GlobalSearchResult suppressedAutomaticPerson = repository.search(
        "default", "John Travolta", 1785000000, 1785400000, 20, 0);
    assert(suppressedAutomaticPerson.recordingTotal == 0);
    assert(suppressedAutomaticPerson.epgTotal == 2);
    assert(suppressedAutomaticPerson.people[0].recordingCount == 0);
    assert(suppressedAutomaticPerson.people[0].epgCount == 2);

    const GlobalSearchResult unicode = repository.search(
        "default", "MÜNCHEN", 1785000000, 1785400000, 20, 0);
    assert(unicode.recordingTotal == 1);
    assert(unicode.epgTotal == 1);

    const GlobalSearchResult isolated = repository.search(
        "second", "Forrest Gump", 1785000000, 1785400000, 20, 0);
    assert(isolated.recordingTotal == 0);
    assert(isolated.epgTotal == 0);

    execute(database,
        "UPDATE suite_metadata_assignments SET assignment_state='withdrawn' "
        "WHERE metadata_assignment_id='manual-1';");

    const GlobalSearchResult withdrawnManual = repository.search(
        "default", "Forrest Gump", 1785000000, 1785400000, 20, 0);
    assert(withdrawnManual.recordingTotal == 0);

    const GlobalSearchResult automaticPerson = repository.search(
        "default", "John Travolta", 1785000000, 1785400000, 20, 0);
    assert(automaticPerson.recordingTotal == 1);
    assert(automaticPerson.epgTotal == 2);
    assert(automaticPerson.recordings[0].matchedPerson == "John Travolta");
    assert(automaticPerson.people[0].recordingCount == 1);

    const GlobalSearchResult recordingSubtitle = repository.search(
        "default", "Director's Cut", 1785000000, 1785400000, 20, 0);
    assert(recordingSubtitle.recordingTotal == 1);
    assert(recordingSubtitle.recordings[0].subtitle == "Director's Cut");

    const GlobalSearchResult isolatedAutomatic = repository.search(
        "second", "Pulp Fiction", 1785000000, 1785400000, 20, 0);
    assert(isolatedAutomatic.recordingTotal == 1);
    assert(isolatedAutomatic.epgTotal == 1);
    assert(isolatedAutomatic.recordings[0].backendId == "second");
    assert(isolatedAutomatic.epg[0].backendId == "second");

    const GlobalSearchResult firstPage = repository.search(
        "default", "John Travolta", 1785000000, 1785400000, 1, 0);
    const GlobalSearchResult secondPage = repository.search(
        "default", "John Travolta", 1785000000, 1785400000, 1, 1);
    assert(firstPage.epg.size() == 1);
    assert(firstPage.epgHasMore);
    assert(secondPage.epg.size() == 1);
    assert(firstPage.epg[0].eventId != secondPage.epg[0].eventId);

    const GlobalSearchResult none = repository.search(
        "default", "Kein Treffer", 1785000000, 1785400000, 20, 0);
    assert(none.recordingTotal == 0);
    assert(none.epgTotal == 0);

    Database performanceDatabase;
    assert(performanceDatabase.open(":memory:"));
    createExistingSchemas(performanceDatabase);
    GlobalSearchRepository performanceRepository(performanceDatabase);
    assert(performanceRepository.ensureSchema());
    execute(performanceDatabase,
        "CREATE INDEX idx_epg_events_backend_end_epoch "
        "ON epg_events(backend_id,CAST(end_time AS INTEGER),CAST(start_time AS INTEGER),channel_id,event_id);"
        "WITH RECURSIVE n(x) AS (VALUES(0) UNION ALL SELECT x+1 FROM n WHERE x<174163) "
        "INSERT INTO epg_events SELECT 'default','channel-'||(x%80),printf('%d',x),"
        "CASE WHEN x=150000 THEN 'Pulp Fiction' ELSE 'Sendung '||(x%12000) END,"
        "'Episode '||(x%500),'',1785000000+x,1785003600+x,3600 FROM n;"
        "INSERT INTO epg_scraper_metadata_people VALUES("
        "'default','channel-'||(150001%80),'150001',0,'actor',"
        "'John Travolta','john travolta','','');");

    const auto performanceStart = std::chrono::steady_clock::now();
    const GlobalSearchResult largeTitle = performanceRepository.search(
        "default", "Pulp Fiction", 1784000000, 1787000000, 24, 0);
    const GlobalSearchResult largePerson = performanceRepository.search(
        "default", "John Travolta", 1784000000, 1787000000, 24, 0);
    const auto performanceMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - performanceStart).count();
    assert(largeTitle.epgTotal == 1);
    assert(largePerson.epgTotal == 1);
    assert(performanceMilliseconds < 5000);

    std::puts("global search repository tests passed");
    return 0;
}
