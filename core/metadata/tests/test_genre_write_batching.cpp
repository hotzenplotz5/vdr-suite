#include "Database.h"
#include "GenreIndexRepository.h"

#include <sqlite3.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

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

// Exercise the same per-file transaction lease used by Agent command polling.
// Inject expensive per-event SQLite work without depending on machine speed.
struct SlowSyncTrace
{
    TransactionCounts counts;
    std::mutex mutex;
    std::condition_variable firstWriteCondition;
    bool firstWrite = false;
    std::atomic<int> targetWrites{0};
};

int traceSlowSync(
    unsigned traceType,
    void* context,
    void* statementPointer,
    void* extra)
{
    if (context == nullptr || statementPointer == nullptr)
        return 0;

    auto& trace = *static_cast<SlowSyncTrace*>(context);
    traceTransactions(traceType, &trace.counts, statementPointer, extra);

    if (traceType != SQLITE_TRACE_STMT) return 0;
    const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statementPointer));
    if (sql == nullptr ||
        std::string(sql).rfind("INSERT INTO suite_metadata_targets ", 0) != 0)
        return 0;

    ++trace.targetWrites;
    {
        std::lock_guard<std::mutex> lock(trace.mutex);
        trace.firstWrite = true;
    }
    trace.firstWriteCondition.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    const std::string& targetId,
    const std::string& genreId = "movie")
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT COUNT(*) "
        "FROM suite_metadata_genre_assignments "
        "WHERE metadata_target_id=? "
        "AND source_kind='epg-browse-content-class' "
        "AND genre_id=? "
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
    assert(sqlite3_bind_text(
        statement, 2, genreId.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK);

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

        // Initial DVB genre synchronization writes the source evidence.
        // Repeating the identical EPG window must not rewrite thousands of
        // unchanged assignments. The retirement step still has one transaction.
        assert(repository.synchronizeEpgCache(
            "default", 1699999999LL, 1700005401LL));
        counts = {};
        assert(sqlite3_trace_v2(
            database.handle(), SQLITE_TRACE_STMT,
            traceTransactions, &counts) == SQLITE_OK);
        assert(repository.synchronizeEpgCache(
            "default", 1699999999LL, 1700005401LL));
        assert(sqlite3_trace_v2(
            database.handle(), 0, nullptr, nullptr) == SQLITE_OK);
        assert(counts.begins == 1);
        assert(counts.commits == 1);
        assert(counts.rollbacks == 0);
        assert(browseAssignmentCount(database.handle(), targetId) == 1);

        // A changed DVB descriptor must still be persisted and reconciled.
        assert(database.execute(
            "UPDATE epg_events SET content_descriptors='Sport' "
            "WHERE backend_id='default' AND channel_id='C1' "
            "AND event_id='100';"));
        counts = {};
        assert(sqlite3_trace_v2(
            database.handle(), SQLITE_TRACE_STMT,
            traceTransactions, &counts) == SQLITE_OK);
        assert(repository.synchronizeEpgCache(
            "default", 1699999999LL, 1700005401LL));
        assert(sqlite3_trace_v2(
            database.handle(), 0, nullptr, nullptr) == SQLITE_OK);
        assert(counts.begins == 2);
        assert(counts.commits == 2);
        assert(counts.rollbacks == 0);
        sqlite3_stmt* changedGenre = nullptr;
        assert(sqlite3_prepare_v2(database.handle(),
            "SELECT COUNT(*) FROM suite_metadata_genre_assignments "
            "WHERE metadata_target_id=? AND provider_id='vdr-epg' "
            "AND original_value='Sport';",
            -1, &changedGenre, nullptr) == SQLITE_OK);
        assert(sqlite3_bind_text(changedGenre, 1, targetId.c_str(),
            -1, SQLITE_TRANSIENT) == SQLITE_OK);
        assert(sqlite3_step(changedGenre) == SQLITE_ROW);
        assert(sqlite3_column_int(changedGenre, 0) > 0);
        sqlite3_finalize(changedGenre);
        // The live EPG browse taxonomy intentionally prioritizes specific
        // DVB sports evidence over a TVScraper movie media-type assignment.
        assert(browseAssignmentCount(
            database.handle(), targetId, "sports") == 1);
        assert(browseAssignmentCount(
            database.handle(), targetId, "movie") == 0);

        // An expired EPG binding should be retired once. Repeating an EPG
        // refresh must not rewrite already retired rows and hold the shared
        // SQLite writer lease for redundant updates.
        assert(repository.retireExpiredEpgTargets("default", 1700005401));
        assert(sqlite3_changes(database.handle()) == 1);
        assert(repository.retireExpiredEpgTargets("default", 1700005401));
        assert(sqlite3_changes(database.handle()) == 0);

        // Stale rows are also eligible; rows already marked retired must
        // remain untouched on subsequent startup cleanups.
        assert(database.execute(
            "UPDATE suite_metadata_target_bindings "
            "SET lifecycle_state='stale' WHERE metadata_target_id='" +
            targetId + "';"));
        assert(repository.retireExpiredEpgTargets("default", 1700005401));
        assert(sqlite3_changes(database.handle()) == 1);
        assert(repository.retireExpiredEpgTargets("default", 1700005401));
        assert(sqlite3_changes(database.handle()) == 0);

        // The existing lookup index has lifecycle_state as the third key.
        // An IN predicate can constrain it; != 'retired' cannot.
        sqlite3_stmt* retirementPlan = nullptr;
        assert(sqlite3_prepare_v2(database.handle(),
            "EXPLAIN QUERY PLAN UPDATE suite_metadata_target_bindings "
            "SET lifecycle_state='retired',updated_at=CURRENT_TIMESTAMP "
            "WHERE backend_id=? AND target_type='program-event' "
            "AND lifecycle_state IN ('active','stale') "
            "AND end_time>0 AND end_time<?;",
            -1, &retirementPlan, nullptr) == SQLITE_OK);
        assert(sqlite3_bind_text(
            retirementPlan, 1, "default", -1, SQLITE_STATIC) == SQLITE_OK);
        assert(sqlite3_bind_int64(retirementPlan, 2, 1700005401) == SQLITE_OK);
        bool usesLifecycleIndex = false;
        while (sqlite3_step(retirementPlan) == SQLITE_ROW)
        {
            const unsigned char* detail =
                sqlite3_column_text(retirementPlan, 3);
            if (detail == nullptr) continue;
            const std::string plan(reinterpret_cast<const char*>(detail));
            if (plan.find("idx_suite_metadata_target_bindings_lookup") !=
                    std::string::npos &&
                plan.find("lifecycle_state=?") != std::string::npos)
            {
                usesLifecycleIndex = true;
            }
        }
        sqlite3_finalize(retirementPlan);
        assert(usesLifecycleIndex);
    }

    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    // Slow EPG evidence writes must release the common SQLite writer lease
    // between rows. A concurrent Agent-style poll must run before all four
    // changed rows are processed, not after one long 16-row transaction.
    const std::string concurrentFilename =
        "/tmp/vdr-suite-genre-lease-budget-test.sqlite";
    std::remove(concurrentFilename.c_str());
    std::remove((concurrentFilename + "-wal").c_str());
    std::remove((concurrentFilename + "-shm").c_str());
    {
        Database epgDatabase;
        Database pollDatabase;
        assert(epgDatabase.open(concurrentFilename));
        assert(pollDatabase.open(concurrentFilename));
        createSourceSchemas(epgDatabase);

        for (int i = 0; i < 4; ++i)
        {
            assert(epgDatabase.execute(
                "INSERT INTO epg_events VALUES("
                "'default','C2','" + std::to_string(200 + i) +
                "','Example movie','','Description',"
                "'1700000000','1700005400',5400,'Film/Thriller');"));
        }

        GenreIndexRepository syncRepository(epgDatabase);
        assert(syncRepository.ensureSchema());

        SlowSyncTrace trace;
        assert(sqlite3_trace_v2(
            epgDatabase.handle(), SQLITE_TRACE_STMT,
            traceSlowSync, &trace) == SQLITE_OK);

        std::atomic<int> writesAtPoll{-1};
        std::thread poll([&]
        {
            std::unique_lock<std::mutex> lock(trace.mutex);
            const bool started = trace.firstWriteCondition.wait_for(
                lock,
                std::chrono::seconds(5),
                [&] { return trace.firstWrite; });
            lock.unlock();
            assert(started);

            // This is the same filename-scoped lease and BEGIN IMMEDIATE
            // path that a non-empty Backend-Agent command poll uses.
            auto lease = pollDatabase.acquireTransactionLease();
            writesAtPoll = trace.targetWrites.load();
            assert(pollDatabase.execute("BEGIN IMMEDIATE TRANSACTION;"));
            assert(pollDatabase.execute("COMMIT;"));
        });

        const bool synchronized = syncRepository.synchronizeEpgCache(
            "default", 1699999999LL, 1700005401LL);
        poll.join();
        assert(sqlite3_trace_v2(
            epgDatabase.handle(), 0, nullptr, nullptr) == SQLITE_OK);
        assert(synchronized);
        assert(trace.targetWrites == 4);
        // Four deliberately slow event writes, each atomic, then retirement.
        assert(trace.counts.begins == 5);
        assert(trace.counts.commits == 5);
        assert(trace.counts.rollbacks == 0);
        assert(writesAtPoll > 0 && writesAtPoll < 4);

        for (int i = 0; i < 4; ++i)
        {
            const std::string targetId =
                GenreIndexRepository::stableTargetId(
                    "program-event", "default",
                    "C2\n" + std::to_string(200 + i));
            assert(browseAssignmentCount(epgDatabase.handle(), targetId) == 1);
        }
    }
    std::remove(concurrentFilename.c_str());
    std::remove((concurrentFilename + "-wal").c_str());
    std::remove((concurrentFilename + "-shm").c_str());

    std::cout << "genre write batching ok\n";
    return 0;
}
