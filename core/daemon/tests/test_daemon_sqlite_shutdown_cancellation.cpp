#include "DaemonSqliteShutdownCancellation.h"

#include <sqlite3.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

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
    assert(execute(
        database,
        "CREATE TABLE events(value INTEGER NOT NULL);"));

    std::atomic<bool> workerStarted(false);
    std::atomic<int> workerStepResult(SQLITE_OK);
    std::atomic<bool> workerFinishedInAutocommit(false);

    std::thread worker([&]() {
        assert(execute(database, "BEGIN IMMEDIATE TRANSACTION;"));

        sqlite3_stmt* statement = nullptr;
        assert(sqlite3_prepare_v2(
                   database,
                   "INSERT INTO events(value) VALUES(?);",
                   -1,
                   &statement,
                   nullptr) == SQLITE_OK);

        workerStarted.store(true);

        for (int value = 0; value < 100000000; ++value)
        {
            sqlite3_reset(statement);
            sqlite3_clear_bindings(statement);
            sqlite3_bind_int(statement, 1, value);

            const int result = sqlite3_step(statement);
            if (result != SQLITE_DONE)
            {
                workerStepResult.store(result);
                break;
            }
        }

        assert(sqlite3_finalize(statement) == SQLITE_INTERRUPT);

        if (sqlite3_get_autocommit(database) == 0)
        {
            assert(execute(database, "ROLLBACK;"));
        }

        workerFinishedInAutocommit.store(
            sqlite3_get_autocommit(database) != 0);
    });

    while (!workerStarted.load())
    {
        std::this_thread::yield();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto cancellationStarted = std::chrono::steady_clock::now();

    {
        DaemonSqliteShutdownCancellation cancellation(database);
        worker.join();
        assert(cancellation.cancellationDelivered());
    }

    const auto cancellationDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - cancellationStarted);

    assert(workerStepResult.load() == SQLITE_INTERRUPT);
    assert(workerFinishedInAutocommit.load());
    assert(cancellationDuration < std::chrono::seconds(2));

    assert(execute(database, "BEGIN; COMMIT;"));
    assert(sqlite3_close(database) == SQLITE_OK);

    std::cout
        << "daemon sqlite shutdown cancellation ok"
        << std::endl;
    return 0;
}
