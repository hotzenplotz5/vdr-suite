#include "Database.h"
#include "MetadataRepository.h"

#include <sqlite3.h>

#include <cassert>
#include <cstring>
#include <map>
#include <string>

namespace
{
ManualRecordingMetadataPerson person(
    const std::string& externalId,
    const std::string& name,
    const std::string& characterName,
    int ordinal)
{
    ManualRecordingMetadataPerson value;
    value.providerId = "tmdb";
    value.externalNamespace = "person";
    value.externalId = externalId;
    value.name = name;
    value.role = "actor";
    value.characterName = characterName;
    value.ordinal = ordinal;
    return value;
}

ManualRecordingMetadataSelection selection(
    const std::string& resourceKey,
    const std::string& externalId,
    const std::string& title,
    int expectedRevision)
{
    ManualRecordingMetadataSelection value;
    value.backendId = "default";
    value.resourceKey = resourceKey;
    value.providerId = "tmdb";
    value.externalNamespace = "movie";
    value.externalId = externalId;
    value.mediaType = "movie";
    value.title = title;
    value.originalTitle = title + " original";
    value.overview = "Manual selection";
    value.releaseDate = "2020-01-01";
    value.actorRef = "user:test-admin";
    value.expectedRevision = expectedRevision;
    value.castComplete = true;
    return value;
}

int scalar(sqlite3* database, const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database,
        sql.c_str(),
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

struct SqlTraceState
{
    int manualBatchReads = 0;
    int manualSchemaStatements = 0;
};

int traceSql(
    unsigned int traceKind,
    void* context,
    void* statement,
    void*)
{
    if (traceKind != SQLITE_TRACE_STMT ||
        context == nullptr || statement == nullptr)
        return 0;

    const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
    if (sql == nullptr) return 0;

    SqlTraceState& state = *static_cast<SqlTraceState*>(context);
    if (std::strstr(sql, "FROM suite_metadata_manual_assignment_values v") &&
        std::strstr(sql, "LEFT JOIN suite_metadata_recording_person_relations r"))
        state.manualBatchReads += 1;
    if (std::strstr(
            sql,
            "CREATE TABLE IF NOT EXISTS suite_metadata_manual_assignment_values"))
        state.manualSchemaStatements += 1;
    return 0;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache("
        "backend_id TEXT NOT NULL,"
        "cache_key TEXT NOT NULL,"
        "backend_native_id TEXT NOT NULL,"
        "PRIMARY KEY(backend_id,cache_key));"
        "INSERT INTO vdr_recording_cache(backend_id,cache_key,backend_native_id) "
        "VALUES('default','video/example.rec','/video/example.rec');"
        "INSERT INTO vdr_recording_cache(backend_id,cache_key,backend_native_id) "
        "VALUES('default','video/other.rec','/video/other.rec');"));

    MetadataRepository repository(database);

    ManualRecordingMetadataSelection firstSelection = selection(
        "/video/example.rec",
        "100",
        "Example",
        0);
    firstSelection.people.push_back(person(
        "31", "Tom Hanks", "Forrest Gump", 0));
    firstSelection.people.push_back(person(
        "32", "Robin Wright", "Jenny Curran", 1));

    ManualRecordingMetadataAssignment first;
    assert(repository.assignManualRecordingMetadata(firstSelection, first));
    assert(first.found);
    assert(first.revision == 1);
    assert(first.resourceKey == "video/example.rec");
    assert(first.castComplete);
    assert(first.people.size() == 2U);
    assert(first.people[0].externalId == "31");
    assert(first.people[0].characterName == "Forrest Gump");
    assert(first.people[1].ordinal == 1);

    ManualRecordingMetadataSelection sharedSelection = selection(
        "/video/other.rec",
        "101",
        "Other",
        0);
    sharedSelection.people.push_back(person(
        "31", "Tom Hanks", "Captain Miller", 0));
    ManualRecordingMetadataAssignment shared;
    assert(repository.assignManualRecordingMetadata(sharedSelection, shared));
    assert(shared.found);
    assert(shared.people.size() == 1U);
    assert(shared.people[0].metadataEntityId == first.people[0].metadataEntityId);
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_person_values;") == 2);
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_entity_external_ids "
        "WHERE provider_id='tmdb' AND external_namespace='person' "
        "AND external_id='31';") == 1);

    ManualRecordingMetadataSelection replacementSelection = selection(
        "/video/example.rec",
        "200",
        "Replacement",
        first.revision);
    replacementSelection.people.push_back(person(
        "33", "Gary Sinise", "Lieutenant Dan Taylor", 0));
    ManualRecordingMetadataAssignment replacement;
    assert(repository.assignManualRecordingMetadata(
        replacementSelection,
        replacement));
    assert(replacement.found);
    assert(replacement.revision == 2);
    assert(replacement.externalId == "200");
    assert(replacement.people.size() == 1U);
    assert(replacement.people[0].externalId == "33");
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_recording_person_relations;") == 4);
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_assignments "
        "WHERE assignment_state='superseded';") == 1);

    ManualRecordingMetadataAssignment stale;
    assert(!repository.assignManualRecordingMetadata(
        selection("/video/example.rec", "300", "Stale", first.revision),
        stale));
    assert(!stale.found);

    {
        MetadataRepository restarted(database);
        const ManualRecordingMetadataAssignment current =
            restarted.getManualRecordingMetadata(
                "default",
                "/video/example.rec");
        assert(current.found);
        assert(current.revision == replacement.revision);
        assert(current.externalId == "200");
        assert(current.people.size() == 1U);
        assert(current.people[0].name == "Gary Sinise");

        const ManualRecordingMetadataAssignment other =
            restarted.getManualRecordingMetadata(
                "default",
                "/video/other.rec");
        assert(other.found);
        assert(other.people.size() == 1U);
        assert(other.people[0].metadataEntityId == first.people[0].metadataEntityId);
    }

    ManualRecordingMetadataAssignment withdrawn;
    assert(repository.withdrawManualRecordingMetadata(
        "default",
        "/video/example.rec",
        "user:test-admin",
        replacement.revision,
        withdrawn));
    assert(!repository.getManualRecordingMetadata(
        "default",
        "/video/example.rec").found);
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_recording_person_relations;") == 4);
    assert(scalar(
        database.handle(),
        "SELECT COUNT(*) FROM suite_metadata_manual_assignment_withdrawals;") == 1);

    SqlTraceState traceState;
    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        traceSql,
        &traceState) == SQLITE_OK);

    const std::map<std::string, ManualRecordingMetadataAssignment> batch =
        repository.getManualRecordingMetadataForBackend("default");
    assert(batch.count("/video/example.rec") == 0);
    assert(batch.count("video/example.rec") == 0);
    assert(batch.count("/video/other.rec") == 1);
    assert(batch.count("video/other.rec") == 1);
    assert(batch.at("/video/other.rec").people.size() == 1U);
    assert(batch.at("video/other.rec").people[0].externalId == "31");
    assert(traceState.manualBatchReads == 1);
    assert(traceState.manualSchemaStatements == 0);

    sqlite3_trace_v2(database.handle(), 0, nullptr, nullptr);
    database.close();
    return 0;
}
