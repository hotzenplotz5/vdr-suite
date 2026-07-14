#include "Database.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

int main()
{
    const char* databasePath = "/tmp/vdr-suite-cpp-test.db";
    std::remove(databasePath);

    Database db;

    if (!db.open(databasePath)) {
        std::cerr << "open failed\n";
        return 1;
    }

    if (!db.isOpen()) {
        std::cerr << "database is not open\n";
        return 1;
    }

    if (!db.execute(
            "CREATE TABLE IF NOT EXISTS test ("
            "id INTEGER PRIMARY KEY, name TEXT);")) {
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

    std::atomic<bool> firstTransactionStarted(false);
    bool firstTransactionOk = false;
    bool secondTransactionOk = false;
    long long secondWaitMilliseconds = 0;

    std::thread firstTransaction([&]() {
        firstTransactionOk =
            db.execute("BEGIN IMMEDIATE TRANSACTION;");

        if (!firstTransactionOk) {
            firstTransactionStarted.store(true);
            return;
        }

        firstTransactionStarted.store(true);

        firstTransactionOk =
            db.execute("INSERT INTO test (name) VALUES ('first');");

        std::this_thread::sleep_for(
            std::chrono::milliseconds(150));

        firstTransactionOk =
            db.execute("COMMIT;") && firstTransactionOk;
    });

    std::thread secondTransaction([&]() {
        while (!firstTransactionStarted.load()) {
            std::this_thread::yield();
        }

        const auto started =
            std::chrono::steady_clock::now();

        secondTransactionOk =
            db.execute("BEGIN IMMEDIATE TRANSACTION;");

        const auto acquired =
            std::chrono::steady_clock::now();

        secondWaitMilliseconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                acquired - started).count();

        if (!secondTransactionOk) {
            return;
        }

        secondTransactionOk =
            db.execute("INSERT INTO test (name) VALUES ('second');");

        secondTransactionOk =
            db.execute("COMMIT;") && secondTransactionOk;
    });

    firstTransaction.join();
    secondTransaction.join();

    assert(firstTransactionOk);
    assert(secondTransactionOk);
    assert(secondWaitMilliseconds >= 100);

    db.close();
    std::remove(databasePath);

    std::cout << "Database test OK\n";
    return 0;
}
