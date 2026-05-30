#include "Database.h"

#include <sqlite3.h>
#include <iostream>

Database::Database()
    : db_(nullptr)
{
}

Database::~Database()
{
    close();
}

bool Database::open(const std::string& filename)
{
    if (sqlite3_open(filename.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "Failed to open database: "
                  << sqlite3_errmsg(db_) << std::endl;
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

bool Database::execute(const std::string& sql)
{
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
