#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <iostream>

namespace
{
std::string firstSqlKeyword(const std::string& sql)
{
    const auto first = std::find_if_not(
        sql.begin(),
        sql.end(),
        [](unsigned char character) {
            return std::isspace(character) != 0;
        });

    std::string keyword;

    for (auto iterator = first;
         iterator != sql.end() &&
         std::isalpha(static_cast<unsigned char>(*iterator)) != 0;
         ++iterator)
    {
        keyword.push_back(
            static_cast<char>(
                std::toupper(static_cast<unsigned char>(*iterator))));
    }

    return keyword;
}
}

Database::Database()
    : db_(nullptr),
      transactionActive_(false),
      transactionOwner_()
{
}

Database::~Database()
{
    close();
}

bool Database::open(const std::string& filename)
{
    std::lock_guard<std::recursive_mutex> lock(connectionMutex_);

    if (db_) {
        close();
    }

    if (sqlite3_open(filename.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database: "
                  << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    transactionActive_ = false;
    transactionOwner_ = std::thread::id();
    return true;
}

void Database::close()
{
    std::lock_guard<std::recursive_mutex> lock(connectionMutex_);

    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }

    transactionActive_ = false;
    transactionOwner_ = std::thread::id();
}

bool Database::isOpen() const
{
    std::lock_guard<std::recursive_mutex> lock(connectionMutex_);
    return db_ != nullptr;
}

bool Database::execute(const std::string& sql)
{
    const std::string keyword = firstSqlKeyword(sql);
    const bool beginsTransaction = keyword == "BEGIN";
    const bool endsTransaction =
        keyword == "COMMIT" || keyword == "ROLLBACK";

    if (beginsTransaction) {
        connectionMutex_.lock();
    }
    else {
        connectionMutex_.lock();
    }

    if (!db_) {
        std::cerr << "Database is not open" << std::endl;
        connectionMutex_.unlock();
        return false;
    }

    char* error = nullptr;

    const int rc = sqlite3_exec(
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

        if (beginsTransaction) {
            connectionMutex_.unlock();
        }

        connectionMutex_.unlock();
        return false;
    }

    if (beginsTransaction) {
        transactionActive_ = true;
        transactionOwner_ = std::this_thread::get_id();
        return true;
    }

    if (endsTransaction &&
        transactionActive_ &&
        transactionOwner_ == std::this_thread::get_id())
    {
        transactionActive_ = false;
        transactionOwner_ = std::thread::id();
        connectionMutex_.unlock();
    }

    connectionMutex_.unlock();
    return true;
}

bool Database::tableExists(const std::string& tableName)
{
    std::lock_guard<std::recursive_mutex> lock(connectionMutex_);

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

    const int rc = sqlite3_exec(
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

sqlite3* Database::handle() const
{
    return db_;
}
