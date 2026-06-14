#pragma once

#include <string>

struct VdrRecording
{
    std::string id;
    std::string backendId;

    std::string title;
    std::string path;

    std::string startTime;

    int durationSeconds;
    long long sizeMb;
};
