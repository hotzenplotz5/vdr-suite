#pragma once

#include "Recording.h"

#include <string>
#include <vector>

class Database;

class RecordingRepository
{
public:
    explicit RecordingRepository(Database& database);

    std::vector<std::string> getRecordingTitles();
    std::vector<Recording> getAllRecordings();

private:
    Database& database_;
};
