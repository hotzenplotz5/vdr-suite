
#pragma once

#include <string>
#include <vector>

struct VdrChannelMoveResult
{
    bool success = false;
    bool dryRun = false;
    std::string backendId = "default";
    int sourceNumber = 0;
    int targetNumber = 0;
    std::string message;
    std::string command;
    std::string rawOutput;
    std::vector<std::string> errors;
};
