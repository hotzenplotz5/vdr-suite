#pragma once

#include "Recording.h"

#include <vector>

class RecordingRepository;

class RecordingService
{
public:
    explicit RecordingService(RecordingRepository& repository);

    std::vector<Recording> getAllRecordings();

private:
    RecordingRepository& repository_;
};
