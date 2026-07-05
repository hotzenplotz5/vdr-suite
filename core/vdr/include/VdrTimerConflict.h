#pragma once

#include <string>
#include <vector>

struct VdrTimerConflictEntry
{
    int timerIndex = 0;
    int percentage = 0;
    std::vector<int> concurrentTimerIndices;
    std::string remoteServer;
};

struct VdrTimerConflict
{
    long long conflictTime = 0;
    std::vector<VdrTimerConflictEntry> entries;
    std::string raw;
};

struct VdrTimerConflictReport
{
    std::string source = "unknown";
    bool available = false;
    bool checkAdvised = false;
    int count = 0;
    int total = 0;
    std::vector<VdrTimerConflict> conflicts;
    std::string error;
};
