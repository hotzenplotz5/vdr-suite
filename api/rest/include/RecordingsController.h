#pragma once

#include "DashboardController.h"

class RecordingRepository;

class RecordingsController
{
public:
    explicit RecordingsController(
        RecordingRepository& recordingRepository);

    ApiResponse getRecordings();

private:
    RecordingRepository& recordingRepository_;
};
