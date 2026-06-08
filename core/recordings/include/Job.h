#pragma once

#include <string>

struct Job
{
    int id = 0;

    int recordingId = 0;

    std::string jobType;
    std::string status;

    int priority = 0;

    std::string message;

    std::string createdAt;
    std::string startedAt;
    std::string finishedAt;
};
