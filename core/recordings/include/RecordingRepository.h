#pragma once

#include <string>
#include <vector>

class Database;

class RecordingRepository
{
public:
    explicit RecordingRepository(Database& database);

    std::vector<std::string> getRecordingTitles();

private:
    Database& database_;
};
