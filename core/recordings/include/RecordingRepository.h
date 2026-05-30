#pragma once

#include "Recording.h"

#include <optional>
#include <string>
#include <vector>

class Database;

class RecordingRepository
{
public:
    explicit RecordingRepository(Database& database);

    std::vector<std::string> getRecordingTitles();
    std::vector<Recording> getAllRecordings();
    std::optional<Recording> getRecordingById(int id);

private:
    Database& database_;
};
