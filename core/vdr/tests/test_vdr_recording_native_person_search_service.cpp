#include "Database.h"
#include "PersonQuery.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativePersonSearchService.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{

VdrRecording makeRecording(
    const std::string& id,
    const std::string& backendNativeId,
    const std::string& title)
{
    VdrRecording recording;
    recording.id = id;
    recording.backendNativeId = backendNativeId;
    recording.title = title;
    recording.path = backendNativeId;
    recording.startTime = "1780000000";
    recording.durationSeconds = 7200;
    recording.sizeMb = 4096;
    return recording;
}

VdrRecordingNativeMetadata makeMetadata(
    const std::string& recordingKey,
    int providerId,
    const std::string& title,
    const std::string& characterName)
{
    VdrRecordingNativeMetadata metadata;
    metadata.availability =
        VdrRecordingNativeMetadataAvailability::Found;
    metadata.schema = 1;
    metadata.found = true;
    metadata.reason = "none";
    metadata.provider = "tvscraper";
    metadata.recordingIdentitySchema = 1;
    metadata.recordingKey = recordingKey;
    metadata.mediaType = "movie";
    metadata.providerId = providerId;
    metadata.title = title;
    metadata.originalTitle = title;

    VdrRecordingNativePerson person;
    person.role = "actor";
    person.name = "Tom Hanks";
    person.characterName = characterName;
    metadata.people.push_back(person);

    return metadata;
}

bool containsTitle(
    const RecordingPersonSearchResult& result,
    const std::string& title)
{
    for (const RecordingPersonSearchMatch& match : result.matches())
    {
        if (match.recording().title == title)
        {
            return true;
        }
    }

    return false;
}

struct TraceState
{
    int effectivePersonReads = 0;
    int schemaStatements = 0;
};

int traceSql(unsigned int kind, void* context, void* statement, void*)
{
    if (kind != SQLITE_TRACE_STMT || context == nullptr || statement == nullptr)
        return 0;
    const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
    if (sql == nullptr) return 0;
    TraceState& state = *static_cast<TraceState*>(context);
    if (std::strstr(sql, "WITH effective_people AS"))
        ++state.effectivePersonReads;
    if (std::strstr(sql, "CREATE TABLE") || std::strstr(sql, "CREATE INDEX"))
        ++state.schemaStatements;
    return 0;
}

void createManualPersonFixtures(
    Database& database,
    const std::string& infernoNativeId)
{
    assert(database.execute(
        "CREATE TABLE suite_metadata_assignments("
        "metadata_assignment_id TEXT PRIMARY KEY,assignment_state TEXT,"
        "manual_assignment INTEGER,relationship_locked INTEGER);"
        "CREATE TABLE suite_metadata_manual_assignment_values("
        "metadata_assignment_id TEXT PRIMARY KEY,backend_id TEXT,"
        "resource_key TEXT,media_type TEXT,title TEXT,original_title TEXT,"
        "poster_reference TEXT);"
        "CREATE TABLE suite_metadata_person_values("
        "metadata_entity_id TEXT PRIMARY KEY,provider_id TEXT,"
        "external_namespace TEXT,external_id TEXT,display_name TEXT,"
        "name_folded TEXT,normalized_name TEXT);"
        "CREATE TABLE suite_metadata_recording_person_relations("
        "metadata_assignment_id TEXT,metadata_target_id TEXT,"
        "person_entity_id TEXT,metadata_evidence_id TEXT,role TEXT,"
        "character_name TEXT,character_name_folded TEXT,ordinal INTEGER);"
        "INSERT INTO suite_metadata_assignments VALUES("
        "'manual-inferno','selected',1,1);"
        "INSERT INTO suite_metadata_person_values VALUES("
        "'tmdb-person-35','tmdb','person','35','Audrey Tautou',"
        "'audrey tautou','audrey-tautou');"
        "INSERT INTO suite_metadata_recording_person_relations VALUES("
        "'manual-inferno','target-inferno','tmdb-person-35','evidence-inferno',"
        "'actor','Sophie Neveu','sophie neveu',0);"));

    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(),
        "INSERT INTO suite_metadata_manual_assignment_values "
        "SELECT 'manual-inferno','default',cache_key,'movie',"
        "'The Da Vinci Code','The Da Vinci Code','' "
        "FROM vdr_recording_cache WHERE backend_id='default' "
        "AND backend_native_id=?;",
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_bind_text(
        statement,
        1,
        infernoNativeId.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_DONE);
    assert(sqlite3_changes(database.handle()) == 1);
    sqlite3_finalize(statement);
}

}

int main()
{
    const char* databasePath =
        "/tmp/test_vdr_recording_native_person_search_service.db";

    std::remove(databasePath);

    Database database;
    assert(database.open(databasePath));

    VdrRecordingCacheRepository recordingCacheRepository(database);
    VdrRecordingNativeMetadataRepository metadataRepository(database);

    assert(recordingCacheRepository.ensureSchema());
    assert(metadataRepository.ensureSchema());

    const std::string infernoNativeId =
        "/srv/vdr/video/Thriller/Inferno/2026-05-21.20.38.1-0.rec";
    const std::string sullyNativeId =
        "/srv/vdr/video/Drama/Sully/2026-06-01.20.15.1-0.rec";
    const std::string castAwayNativeId =
        "/srv/vdr/video/Filme/Cast_Away/2026-06-02.20.15.1-0.rec";

    assert(recordingCacheRepository.replaceRecordingsForBackend(
        "default",
        {
            makeRecording("inferno", infernoNativeId, "Thriller/Inferno"),
            makeRecording("sully", sullyNativeId, "Drama/Sully")
        }));

    assert(recordingCacheRepository.replaceRecordingsForBackend(
        "remote",
        {
            makeRecording("cast-away", castAwayNativeId, "Filme/Cast Away")
        }));

    assert(metadataRepository.storeResolution(
        "default",
        infernoNativeId,
        makeMetadata(
            "11111111111111111111111111111111",
            207932,
            "Inferno",
            "Robert Langdon"),
        1000,
        2000,
        0));

    assert(metadataRepository.storeResolution(
        "default",
        sullyNativeId,
        makeMetadata(
            "22222222222222222222222222222222",
            363676,
            "Sully",
            "Chesley Sullenberger"),
        1000,
        2000,
        0));

    assert(metadataRepository.storeResolution(
        "remote",
        castAwayNativeId,
        makeMetadata(
            "33333333333333333333333333333333",
            8358,
            "Cast Away",
            "Chuck Noland"),
        1000,
        2000,
        0));

    VdrRecordingNativePersonSearchService service(
        metadataRepository,
        recordingCacheRepository);

    const RecordingPersonSearchResult byName =
        service.search(
            "default",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);

    assert(byName.totalCount() == 2);
    assert(byName.returnedCount() == 2);
    assert(containsTitle(byName, "Thriller/Inferno"));
    assert(containsTitle(byName, "Drama/Sully"));

    const RecordingPersonSearchResult byNormalizedName =
        service.search(
            "default",
            PersonQuery::byNormalizedName("tom-hanks"),
            20,
            0);

    assert(byNormalizedName.totalCount() == 2);

    const RecordingPersonSearchResult byCharacter =
        service.search(
            "default",
            PersonQuery::byCharacterName("Langdon"),
            20,
            0);

    assert(byCharacter.totalCount() == 1);
    assert(byCharacter.returnedCount() == 1);
    assert(byCharacter.matches().front().recording().title ==
        "Thriller/Inferno");
    assert(byCharacter.matches().front().person().characterName() ==
        "Robert Langdon");

    const RecordingPersonSearchResult byRole =
        service.search(
            "default",
            PersonQuery::byRole(PersonRole::Actor),
            20,
            0);

    assert(byRole.totalCount() == 2);

    const RecordingPersonSearchResult page =
        service.search(
            "default",
            PersonQuery::byName("Hanks"),
            1,
            1);

    assert(page.totalCount() == 2);
    assert(page.returnedCount() == 1);
    assert(page.limit() == 1);
    assert(page.offset() == 1);

    const RecordingPersonSearchResult remote =
        service.search(
            "remote",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);

    assert(remote.totalCount() == 1);
    assert(remote.returnedCount() == 1);
    assert(remote.matches().front().recording().backendId == "remote");
    assert(remote.matches().front().recording().title == "Filme/Cast Away");

    PersonQuery wrongSource = PersonQuery::byName("Tom Hanks");
    wrongSource.withSource(ContentClassificationSource::Tmdb);

    assert(service.search("default", wrongSource, 20, 0).empty());

    assert(service.search(
        "default",
        PersonQuery::byProviderReference("tmdb:31"),
        20,
        0).empty());

    createManualPersonFixtures(database, infernoNativeId);

    TraceState traceState;
    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        traceSql,
        &traceState) == SQLITE_OK);

    PersonQuery manualQuery = PersonQuery::byName("Audrey Tautou");
    manualQuery.withSource(ContentClassificationSource::Tmdb);
    const RecordingPersonSearchResult manual =
        service.search("default", manualQuery, 20, 0);
    assert(manual.totalCount() == 1);
    assert(manual.returnedCount() == 1);
    assert(manual.matches()[0].recording().title == "Thriller/Inferno");
    assert(manual.matches()[0].person().source() ==
        ContentClassificationSource::Tmdb);
    assert(manual.matches()[0].person().characterName() == "Sophie Neveu");
    assert(manual.matches()[0].person().providerReference() ==
        "tmdb:person:35");
    assert(traceState.effectivePersonReads == 2);
    assert(traceState.schemaStatements == 0);

    sqlite3_trace_v2(database.handle(), 0, nullptr, nullptr);

    const RecordingPersonSearchResult manualCharacter =
        service.search(
            "default",
            PersonQuery::byCharacterName("Sophie"),
            20,
            0);
    assert(manualCharacter.totalCount() == 1);
    assert(manualCharacter.matches()[0].person().name() == "Audrey Tautou");

    assert(service.search(
        "remote",
        PersonQuery::byName("Audrey Tautou"),
        20,
        0).empty());

    const RecordingPersonSearchResult automaticSuppressed =
        service.search(
            "default",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);
    assert(automaticSuppressed.totalCount() == 1);
    assert(automaticSuppressed.matches()[0].recording().title == "Drama/Sully");

    assert(database.execute(
        "UPDATE suite_metadata_assignments SET assignment_state='withdrawn' "
        "WHERE metadata_assignment_id='manual-inferno';"));

    assert(service.search(
        "default",
        PersonQuery::byName("Audrey Tautou"),
        20,
        0).empty());
    const RecordingPersonSearchResult automaticRestored =
        service.search(
            "default",
            PersonQuery::byName("Tom Hanks"),
            20,
            0);
    assert(automaticRestored.totalCount() == 2);
    assert(containsTitle(automaticRestored, "Thriller/Inferno"));
    assert(containsTitle(automaticRestored, "Drama/Sully"));

    database.close();
    std::remove(databasePath);

    std::cout
        << "test_vdr_recording_native_person_search_service passed"
        << std::endl;

    return 0;
}
