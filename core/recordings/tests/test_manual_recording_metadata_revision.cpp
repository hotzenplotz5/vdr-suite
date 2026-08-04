#include "Database.h"
#include "MetadataRepository.h"

#include <sqlite3.h>

#include <cassert>
#include <cstring>
#include <map>

namespace
{
ManualRecordingMetadataSelection selection(
    const std::string& externalId,
    int expectedRevision)
{
    ManualRecordingMetadataSelection value;
    value.backendId = "default";
    value.resourceKey = "/video/example.rec";
    value.providerId = "tmdb";
    value.externalNamespace = "movie";
    value.externalId = externalId;
    value.mediaType = "movie";
    value.title = "Example";
    value.originalTitle = "Example";
    value.overview = "Manual selection";
    value.releaseDate = "2020-01-01";
    value.actorRef = "user:test-admin";
    value.expectedRevision = expectedRevision;
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
        std::strstr(sql, "LEFT JOIN vdr_recording_cache c"))
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
        "VALUES('default','video/example.rec','/video/example.rec');"));

    MetadataRepository repository(database);

    ManualRecordingMetadataAssignment first;
    assert(repository.assignManualRecordingMetadata(
        selection("100", 0),
        first));
    assert(first.found);
    assert(first.revision == 1);
    assert(first.resourceKey == "video/example.rec");

    ManualRecordingMetadataAssignment withdrawn;
    assert(repository.withdrawManualRecordingMetadata(
        "default",
        "/video/example.rec",
        "user:test-admin",
        first.revision,
        withdrawn));

    ManualRecordingMetadataAssignment second;
    assert(repository.assignManualRecordingMetadata(
        selection("200", 0),
        second));
    assert(second.found);
    assert(second.revision == 2);
    assert(second.externalId == "200");
    assert(second.resourceKey == "video/example.rec");

    ManualRecordingMetadataAssignment stale;
    assert(!repository.assignManualRecordingMetadata(
        selection("300", first.revision),
        stale));
    assert(!stale.found);

    SqlTraceState traceState;
    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        traceSql,
        &traceState) == SQLITE_OK);

    const ManualRecordingMetadataAssignment current =
        repository.getManualRecordingMetadata(
            "default",
            "/video/example.rec");
    assert(current.found);
    assert(current.revision == second.revision);
    assert(current.externalId == "200");

    const std::map<std::string, ManualRecordingMetadataAssignment> batch =
        repository.getManualRecordingMetadataForBackend("default");
    assert(batch.count("/video/example.rec") == 1);
    assert(batch.count("video/example.rec") == 1);
    assert(batch.at("/video/example.rec").revision == second.revision);
    assert(batch.at("video/example.rec").externalId == "200");
    assert(traceState.manualBatchReads == 1);
    assert(traceState.manualSchemaStatements == 0);

    sqlite3_trace_v2(database.handle(), 0, nullptr, nullptr);
    database.close();
    return 0;
}
