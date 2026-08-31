#include "Database.h"
#include "GenreIndexRepository.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
constexpr int RecordingCount = 1003;

struct WriteCounts
{
    int begins = 0;
    int commits = 0;
    int rollbacks = 0;
    int inserts = 0;
    int updates = 0;
    int deletes = 0;
};

void bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    assert(sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK);
}

std::string trimmedSql(const char* sql)
{
    if (sql == nullptr) return {};

    const std::string value(sql);
    const std::size_t first =
        value.find_first_not_of(" \t\r\n");

    return first == std::string::npos
        ? std::string()
        : value.substr(first);
}

int traceWrites(
    unsigned traceType,
    void* context,
    void* statementPointer,
    void*)
{
    if (traceType != SQLITE_TRACE_STMT ||
        context == nullptr ||
        statementPointer == nullptr)
    {
        return 0;
    }

    const std::string sql = trimmedSql(
        sqlite3_sql(
            static_cast<sqlite3_stmt*>(
                statementPointer)));

    WriteCounts& counts =
        *static_cast<WriteCounts*>(context);

    if (sql.rfind("BEGIN IMMEDIATE TRANSACTION", 0) == 0)
    {
        ++counts.begins;
    }
    else if (sql.rfind("COMMIT", 0) == 0)
    {
        ++counts.commits;
    }
    else if (sql.rfind("ROLLBACK", 0) == 0)
    {
        ++counts.rollbacks;
    }
    else if (sql.rfind("INSERT", 0) == 0)
    {
        ++counts.inserts;
    }
    else if (sql.rfind("UPDATE", 0) == 0)
    {
        ++counts.updates;
    }
    else if (sql.rfind("DELETE", 0) == 0)
    {
        ++counts.deletes;
    }

    return 0;
}

void createSourceSchemas(Database& database)
{
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache("
        "backend_id TEXT,cache_key TEXT,recording_id TEXT,"
        "backend_native_id TEXT,title TEXT,path TEXT,"
        "start_time TEXT,duration_seconds INTEGER,"
        "size_mb INTEGER,metadata_payload TEXT,"
        "PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata("
        "backend_id TEXT,recording_key TEXT,"
        "backend_native_id TEXT,content_state TEXT,"
        "last_attempt_state TEXT,provider TEXT,media_type TEXT,"
        "PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_text_list("
        "backend_id TEXT,recording_key TEXT,kind TEXT,"
        "ordinal INTEGER,value TEXT,"
        "PRIMARY KEY("
        "backend_id,recording_key,kind,ordinal));"
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,"
        "title TEXT,subtitle TEXT,description TEXT,"
        "start_time TEXT,end_time TEXT,"
        "duration_seconds INTEGER,"
        "content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"));
}

void seedRecordings(Database& database)
{
    assert(database.execute(
        "BEGIN IMMEDIATE TRANSACTION;"));

    sqlite3_stmt* cache = nullptr;
    sqlite3_stmt* metadata = nullptr;
    sqlite3_stmt* genre = nullptr;

    assert(sqlite3_prepare_v2(
        database.handle(),
        "INSERT INTO vdr_recording_cache VALUES("
        "'default',?,?,?,?,?,'100',3600,1000,'{}');",
        -1,
        &cache,
        nullptr) == SQLITE_OK);

    assert(sqlite3_prepare_v2(
        database.handle(),
        "INSERT INTO vdr_recording_native_metadata "
        "VALUES('default',?,?,'found','success','tvscraper','');",
        -1,
        &metadata,
        nullptr) == SQLITE_OK);

    assert(sqlite3_prepare_v2(
        database.handle(),
        "INSERT INTO vdr_recording_native_text_list "
        "VALUES('default',?,'genre',?,?);",
        -1,
        &genre,
        nullptr) == SQLITE_OK);

    for (int index = 0; index < RecordingCount; ++index)
    {
        const std::string suffix = std::to_string(index);
        const std::string cacheKey = "r" + suffix;
        const std::string nativeId = "native" + suffix;
        const bool folderFallback = index % 2 == 0;

        const std::string title = folderFallback
            ? "Action/Recording " + suffix
            : "Recording " + suffix;
        const std::string path = folderFallback
            ? "Action/Recording " + suffix
            : "Movies/Recording " + suffix;

        sqlite3_reset(cache);
        sqlite3_clear_bindings(cache);
        bindText(cache, 1, cacheKey);
        bindText(cache, 2, "id" + suffix);
        bindText(cache, 3, nativeId);
        bindText(cache, 4, title);
        bindText(cache, 5, path);
        assert(sqlite3_step(cache) == SQLITE_DONE);

        if (folderFallback) continue;

        const std::string recordingKey = "m" + suffix;

        sqlite3_reset(metadata);
        sqlite3_clear_bindings(metadata);
        bindText(metadata, 1, recordingKey);
        bindText(metadata, 2, nativeId);
        assert(sqlite3_step(metadata) == SQLITE_DONE);

        sqlite3_reset(genre);
        sqlite3_clear_bindings(genre);
        bindText(genre, 1, recordingKey);
        assert(sqlite3_bind_int(
            genre,
            2,
            0) == SQLITE_OK);
        bindText(genre, 3, "Drama");
        assert(sqlite3_step(genre) == SQLITE_DONE);

        if (index % 4 != 1) continue;

        sqlite3_reset(genre);
        sqlite3_clear_bindings(genre);
        bindText(genre, 1, recordingKey);
        assert(sqlite3_bind_int(
            genre,
            2,
            1) == SQLITE_OK);
        bindText(genre, 3, "Thriller");
        assert(sqlite3_step(genre) == SQLITE_DONE);
    }

    assert(sqlite3_finalize(cache) == SQLITE_OK);
    assert(sqlite3_finalize(metadata) == SQLITE_OK);
    assert(sqlite3_finalize(genre) == SQLITE_OK);
    assert(database.execute("COMMIT;"));
}

WriteCounts tracedSynchronization(
    Database& database,
    GenreIndexRepository& repository)
{
    WriteCounts counts;

    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        traceWrites,
        &counts) == SQLITE_OK);

    const bool synchronized =
        repository.synchronizeRecordingCache("default");

    assert(sqlite3_trace_v2(
        database.handle(),
        0,
        nullptr,
        nullptr) == SQLITE_OK);

    assert(synchronized);
    return counts;
}

void assertNoWrites(const WriteCounts& counts)
{
    assert(counts.begins == 0);
    assert(counts.commits == 0);
    assert(counts.rollbacks == 0);
    assert(counts.inserts == 0);
    assert(counts.updates == 0);
    assert(counts.deletes == 0);
}

int activeBindingCount(sqlite3* database)
{
    sqlite3_stmt* statement = nullptr;

    assert(sqlite3_prepare_v2(
        database,
        "SELECT COUNT(*) "
        "FROM suite_metadata_target_bindings "
        "WHERE backend_id='default' "
        "AND target_type='recording' "
        "AND lifecycle_state='active';",
        -1,
        &statement,
        nullptr) == SQLITE_OK);

    int count = 0;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        count = sqlite3_column_int(statement, 0);
    }

    sqlite3_finalize(statement);
    return count;
}
}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-recording-sync-noop.sqlite";

    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    {
        Database database;
        assert(database.open(filename));

        createSourceSchemas(database);
        seedRecordings(database);

        GenreIndexRepository repository(database);
        assert(repository.ensureSchema());

        assert(repository.synchronizeRecordingCache(
            "default"));
        assert(activeBindingCount(
            database.handle()) == RecordingCount);

        const WriteCounts firstNoop =
            tracedSynchronization(database, repository);
        assertNoWrites(firstNoop);

        assert(database.execute(
            "UPDATE vdr_recording_native_text_list "
            "SET value='Comedy' "
            "WHERE backend_id='default' "
            "AND recording_key='m1' "
            "AND kind='genre' AND ordinal=0;"));

        const WriteCounts changed =
            tracedSynchronization(database, repository);

        assert(changed.begins == 1);
        assert(changed.commits == 1);
        assert(changed.rollbacks == 0);
        assert(changed.inserts > 0);
        assert(changed.updates > 0);
        assert(changed.deletes > 0);

        const WriteCounts secondNoop =
            tracedSynchronization(database, repository);
        assertNoWrites(secondNoop);

        assert(database.execute(
            "DELETE FROM vdr_recording_cache "
            "WHERE backend_id='default' "
            "AND cache_key='r1002';"));

        const WriteCounts retired =
            tracedSynchronization(database, repository);

        assert(retired.begins == 1);
        assert(retired.commits == 1);
        assert(retired.rollbacks == 0);
        assert(retired.updates > 0);
        assert(activeBindingCount(
            database.handle()) == RecordingCount - 1);

        const WriteCounts retirementNoop =
            tracedSynchronization(database, repository);
        assertNoWrites(retirementNoop);
    }

    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    std::cout << "genre recording sync noop ok\n";
    return 0;
}