#include "Database.h"
#include "GenreIndexRepository.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
struct TransactionCounts
{
    int begins = 0;
    int commits = 0;
    int rollbacks = 0;
};

int traceTransactions(
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

    const char* sql = sqlite3_sql(
        static_cast<sqlite3_stmt*>(statementPointer));
    if (sql == nullptr)
    {
        return 0;
    }

    TransactionCounts& counts =
        *static_cast<TransactionCounts*>(context);
    const std::string text(sql);

    if (text.rfind("BEGIN IMMEDIATE TRANSACTION", 0) == 0)
    {
        ++counts.begins;
    }
    else if (text.rfind("COMMIT", 0) == 0)
    {
        ++counts.commits;
    }
    else if (text.rfind("ROLLBACK", 0) == 0)
    {
        ++counts.rollbacks;
    }

    return 0;
}

void createSourceSchemas(Database& database)
{
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache("
        "backend_id TEXT,cache_key TEXT,recording_id TEXT,"
        "backend_native_id TEXT,title TEXT,path TEXT,start_time TEXT,"
        "duration_seconds INTEGER,size_mb INTEGER,metadata_payload TEXT,"
        "PRIMARY KEY(backend_id,cache_key));"
        "CREATE TABLE vdr_recording_native_metadata("
        "backend_id TEXT,recording_key TEXT,backend_native_id TEXT,"
        "content_state TEXT,last_attempt_state TEXT,provider TEXT,"
        "PRIMARY KEY(backend_id,recording_key));"
        "CREATE TABLE vdr_recording_native_text_list("
        "backend_id TEXT,recording_key TEXT,kind TEXT,ordinal INTEGER,"
        "value TEXT,PRIMARY KEY(backend_id,recording_key,kind,ordinal));"
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,"
        "subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,"
        "duration_seconds INTEGER,content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"));
}

int browseAssignmentCount(
    sqlite3* database,
    const std::string& targetId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT COUNT(*) "
        "FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? "
        "AND source_kind='epg-browse-content-class' "
        "AND genre_id='movie' "
        "AND assignment_state='active';";

    assert(sqlite3_prepare_v2(
        database,
        sql,
        -1,
        &statement,
        nullptr) == SQLITE_OK);
    assert(sqlite3_bind_text(
        statement,
        1,
        targetId.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK);

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
        "/tmp/vdr-suite-genre-write-batching-test.sqlite";

    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    {
        Database database;
        assert(database.open(filename));
        createSourceSchemas(database);

        assert(database.execute(
            "INSERT INTO epg_events VALUES("
            "'default','C1','100','Example movie','',"
            "'Description','1700000000','1700005400',"
            "5400,'Film/Thriller');"));

        GenreIndexRepository repository(database);
        assert(repository.ensureSchema());

        GenreEvidenceInput genreEvidence;
        genreEvidence.backendId = "default";
        genreEvidence.targetType = "program-event";
        genreEvidence.resourceKey = "C1\n100";
        genreEvidence.nativeId = "100";
        genreEvidence.channelId = "C1";
        genreEvidence.startTime = 1700000000;
        genreEvidence.endTime = 1700005400;
        genreEvidence.providerId = "tvscraper";
        genreEvidence.sourceKind = "scraper-metadata";
        genreEvidence.originalValues = {"Thriller"};
        genreEvidence.state = "active";
        genreEvidence.confidence = 0.95;
        genreEvidence.observedAt = 1700000100;

        GenreEvidenceInput mediaTypeEvidence = genreEvidence;
        mediaTypeEvidence.providerId = "tvscraper-media-type";
        mediaTypeEvidence.sourceKind = "scraper-media-type";
        mediaTypeEvidence.originalValues = {"Movie"};
        mediaTypeEvidence.confidence = 0.99;

        TransactionCounts counts;
        assert(sqlite3_trace_v2(
            database.handle(),
            SQLITE_TRACE_STMT,
            traceTransactions,
            &counts) == SQLITE_OK);

        assert(repository.replaceEpgEvidenceAndReconcile(
            genreEvidence,
            mediaTypeEvidence));

        assert(sqlite3_trace_v2(
            database.handle(),
            0,
            nullptr,
            nullptr) == SQLITE_OK);

        assert(counts.begins == 1);
        assert(counts.commits == 1);
        assert(counts.rollbacks == 0);

        const std::string targetId =
            GenreIndexRepository::stableTargetId(
                "program-event",
                "default",
                "C1\n100");
        assert(browseAssignmentCount(
            database.handle(),
            targetId) == 1);
    }

    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    std::cout << "genre write batching ok\n";
    return 0;
}
