#pragma once

#include <string>

struct VdrRecording
{
    std::string id;

    std::string title;
    std::string path;

    std::string startTime;

    int durationSeconds;
    long long sizeMb;
};
