#include "Database.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-cpp-test.db")) {
        std::cerr << "open failed\n";
        return 1;
    }

    if (!db.isOpen()) {
        std::cerr << "database is not open\n";
        return 1;
    }

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

    std::cout << "Database test OK\n";
    return 0;
}
