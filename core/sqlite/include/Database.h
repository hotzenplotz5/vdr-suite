#pragma once

#include <string>

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
};
