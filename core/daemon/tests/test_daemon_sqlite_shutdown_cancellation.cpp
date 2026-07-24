#include "DaemonSqliteShutdownCancellation.h"

#include <sqlite3.h>

#include <cassert>
#include <iostream>

namespace
{
bool execute(sqlite3* database, const char* sql)
{
    char* error = nullptr;
    const int result = sqlite3_exec(
        database,
        sql,
        nullptr,
        nullptr,
        &error);
    sqlite3_free(error);
    return result == SQLITE_OK;
}
}

int main()
{
    sqlite3* database = nullptr;
    assert(sqlite3_open(":memory:", &database) == SQLITE_OK);
    assert(database != nullptr);
    assert(execute(database, "BEGIN IMMEDIATE TRANSACTION;"));

    {
        DaemonSqliteShutdownCancellation cancellation(database);

        sqlite3_stmt* statement = nullptr;
        const char* sql =
            "WITH RECURSIVE counter(value) AS ("
            "VALUES(0) UNION ALL SELECT value + 1 FROM counter "
            "WHERE value < 100000000"
            ") SELECT sum(value) FROM counter;";
        assert(sqlite3_prepare_v2(
                   database,
                   sql,
                   -1,
                   &statement,
                   nullptr) == SQLITE_OK);

        assert(sqlite3_step(statement) == SQLITE_INTERRUPT);
        assert(cancellation.cancellationDelivered());
        assert(sqlite3_finalize(statement) == SQLITE_INTERRUPT);

        assert(execute(database, "ROLLBACK;"));
    }

    assert(execute(database, "BEGIN; COMMIT;"));
    assert(sqlite3_close(database) == SQLITE_OK);

    std::cout
        << "daemon sqlite shutdown cancellation ok"
        << std::endl;
    return 0;
}
