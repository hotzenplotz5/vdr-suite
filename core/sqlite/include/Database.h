#pragma once

#include <mutex>
#include <string>

struct sqlite3;

class Database {
public:
    using TransactionLease = std::unique_lock<std::recursive_mutex>;

    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    bool open(const std::string& filename);
    void close();

    bool isOpen() const;
    std::string filename() const;

    bool execute(const std::string& sql);
    bool tableExists(const std::string& tableName);

    TransactionLease acquireTransactionLease();

    sqlite3* handle() const;

private:
    sqlite3* db_;
    mutable std::recursive_mutex transactionMutex_;
};
