#include "Database.h"

#include <sqlite3.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>

namespace
{
constexpr int kBusyTimeoutMilliseconds = 5000;

std::recursive_mutex& transactionMutexForFilename(
    const std::string& filename)
{
    static std::mutex registryMutex;
    static std::map<
        std::string,
        std::unique_ptr<std::recursive_mutex>> transactionMutexes;

    std::lock_guard<std::mutex> registryLock(registryMutex);
    auto& mutex = transactionMutexes[filename];
    if (!mutex)
    {
        mutex = std::make_unique<std::recursive_mutex>();
    }
    return *mutex;
}
}

Database::Database()
    : db_(nullptr)
{
}

Database::~Database()
{
    close();
}

Database::Database(Database&& other) noexcept
    : db_(nullptr)
{
    std::lock_guard<std::recursive_mutex> lock(other.transactionMutex_);
    db_ = other.db_;
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    std::scoped_lock lock(transactionMutex_, other.transactionMutex_);

    if (db_)
    {
        sqlite3_close(db_);
    }

    db_ = other.db_;
    other.db_ = nullptr;
    return *this;
}

bool Database::open(const std::string& filename)
{
    if (db_) {
        close();
    }

    if (sqlite3_open(filename.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database: "
                  << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    if (sqlite3_busy_timeout(db_, kBusyTimeoutMilliseconds) != SQLITE_OK)
    {
        std::cerr << "Failed to configure SQLite busy timeout: "
                  << sqlite3_errmsg(db_) << std::endl;
        close();
        return false;
    }

    return true;
}

void Database::close()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::isOpen() const
{
    return db_ != nullptr;
}

std::string Database::filename() const
{
    if (!db_)
    {
        return {};
    }

    const char* value = sqlite3_db_filename(db_, "main");
    return value == nullptr ? std::string() : std::string(value);
}

bool Database::execute(const std::string& sql)
{
    if (!db_) {
        std::cerr << "Database is not open" << std::endl;
        return false;
    }

    char* error = nullptr;

    int rc = sqlite3_exec(
        db_,
        sql.c_str(),
        nullptr,
        nullptr,
        &error
    );

    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: "
                  << (error ? error : "unknown")
                  << std::endl;

        sqlite3_free(error);
        return false;
    }

    return true;
}

bool Database::tableExists(const std::string& tableName)
{
    if (!db_) {
        return false;
    }

    const std::string sql =
        "SELECT name FROM sqlite_master WHERE type='table' AND name='" +
        tableName + "' LIMIT 1;";

    bool found = false;

    auto callback = [](void* data, int, char**, char**) -> int {
        bool* foundPtr = static_cast<bool*>(data);
        *foundPtr = true;
        return 0;
    };

    char* error = nullptr;

    int rc = sqlite3_exec(
        db_,
        sql.c_str(),
        callback,
        &found,
        &error
    );

    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: "
                  << (error ? error : "unknown")
                  << std::endl;

        sqlite3_free(error);
        return false;
    }

    return found;
}

Database::TransactionLease Database::acquireTransactionLease()
{
    const std::string databaseFilename = filename();
    if (databaseFilename.empty())
    {
        return TransactionLease(transactionMutex_);
    }

    // The daemon intentionally owns more than one SQLite connection to the
    // same state file (notably the HTTP security/accountability connection and
    // the primary runtime connection). SQLite permits only one writer at a
    // time, so per-Database mutexes allow those in-process writers to race and
    // surface SQLITE_BUSY despite the normal busy timeout. Key the transaction
    // lease by the actual SQLite filename so same-file writers serialize before
    // they enter SQLite, while genuinely separate database files remain
    // independent.
    return TransactionLease(
        transactionMutexForFilename(databaseFilename));
}

sqlite3* Database::handle() const
{
    return db_;
}
