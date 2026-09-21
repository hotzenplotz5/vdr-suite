#include "Database.h"
#include "GenreIndexRepository.h"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace
{

int captureCandidateSql(
    unsigned traceType,
    void* context,
    void* statement,
    void*)
{
    if (traceType != SQLITE_TRACE_STMT ||
        context == nullptr ||
        statement == nullptr)
    {
        return 0;
    }

    const char* sql = sqlite3_sql(static_cast<sqlite3_stmt*>(statement));
    if (sql != nullptr &&
        std::string(sql).find("FROM epg_events e WHERE") != std::string::npos)
    {
        *static_cast<std::string*>(context) = sql;
    }

    return 0;
}

}

int main()
{
    const std::string filename =
        "/tmp/vdr-suite-genre-epg-refresh-fast-path-test.sqlite";

    std::remove(filename.c_str());

    Database database;
    assert(database.open(filename));
    assert(database.execute(
        "CREATE TABLE epg_events("
        "backend_id TEXT,channel_id TEXT,event_id TEXT,title TEXT,"
        "subtitle TEXT,description TEXT,start_time TEXT,end_time TEXT,"
        "duration_seconds INTEGER,content_descriptors TEXT,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_events VALUES"
        "('a','C1','1','Current epoch event','','',"
        "'1700000100','1700000200',100,''),"
        "('a','C2','2','Ends at lower boundary','','',"
        "'1699999900','1700000000',100,''),"
        "('a','C3','3','Starts at upper boundary','','',"
        "'1700000300','1700000400',100,''),"
        "('a','C4','4','Malformed ten-character time','','',"
        "'0000000000','zzzzzzzzzz',100,'');"));

    GenreIndexRepository repository(database);
    assert(repository.ensureSchema());

    std::string candidateSql;
    assert(sqlite3_trace_v2(
        database.handle(),
        SQLITE_TRACE_STMT,
        captureCandidateSql,
        &candidateSql) == SQLITE_OK);

    const std::vector<GenreEpgRefreshCandidate> candidates =
        repository.epgRefreshCandidates(
            "a",
            1700000000LL,
            1700000300LL,
            "fast-path-test",
            0,
            64);

    assert(candidates.size() == 1);
    assert(candidates.front().eventId == "1");
    assert(!candidateSql.empty());
    assert(candidateSql.find("length(e.end_time)=10") != std::string::npos);
    assert(candidateSql.find("length(e.start_time)=10") != std::string::npos);
    assert(candidateSql.find(
        "e.end_time NOT GLOB '*[^0-9]*'") != std::string::npos);
    assert(candidateSql.find(
        "e.start_time NOT GLOB '*[^0-9]*'") != std::string::npos);
    assert(candidateSql.find("CAST(e.end_time AS INTEGER)") == std::string::npos);
    assert(candidateSql.find("CAST(e.start_time AS INTEGER)") == std::string::npos);

    assert(sqlite3_trace_v2(
        database.handle(),
        0,
        nullptr,
        nullptr) == SQLITE_OK);

    database.close();
    std::remove(filename.c_str());
    std::remove((filename + "-wal").c_str());
    std::remove((filename + "-shm").c_str());

    std::cout << "genre epg refresh fast path ok\n";
    return 0;
}
