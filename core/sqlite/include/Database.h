#pragma once

#include <mutex>
#include <string>
#include <thread>

struct sqlite3;

class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& filename);
    void close();

    bool isOpen() const;

    bool execute(const std::string& sql);
    bool tableExists(const std::string& tableName);

    sqlite3* handle() const;

private:
    sqlite3* db_;
    mutable std::recursive_mutex connectionMutex_;
    bool transactionActive_;
    std::thread::id transactionOwner_;
};
