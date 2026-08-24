#include "Database.h"

#include <sqlite3.h>

#include <cstdio>
#include <iostream>

int main()
{
    const char* path = "/tmp/vdr-suite-cpp-test.db";
    std::remove(path);

    Database db;

    if (!db.open(path)) {
        std::cerr << "open failed\n";
        return 1;
    }

    if (!db.isOpen()) {
        std::cerr << "database is not open\n";
        return 1;
    }

    sqlite3_stmt* busyTimeoutStatement = nullptr;
    if (sqlite3_prepare_v2(
            db.handle(),
            "PRAGMA busy_timeout;",
            -1,
            &busyTimeoutStatement,
            nullptr) != SQLITE_OK ||
        sqlite3_step(busyTimeoutStatement) != SQLITE_ROW ||
        sqlite3_column_int(busyTimeoutStatement, 0) != 5000)
    {
        std::cerr << "busy timeout was not configured\n";
        sqlite3_finalize(busyTimeoutStatement);
        return 1;
    }
    sqlite3_finalize(busyTimeoutStatement);

    if (!db.execute("CREATE TABLE IF NOT EXISTS test (id INTEGER PRIMARY KEY, name TEXT);")) {
        std::cerr << "create table failed\n";
        return 1;
    }

    if (!db.tableExists("test")) {
        std::cerr << "tableExists failed\n";
        return 1;
    }

    if (!db.execute("INSERT INTO test (name) VALUES ('VDR-Suite');")) {
        std::cerr << "insert failed\n";
        return 1;
    }

    db.close();
    std::remove(path);

    std::cout << "Database test OK\n";
    return 0;
}