#pragma once

#include <cstdint>
#include <string>

struct VdrStatus
{
    bool enabled;
    std::string mode;
    std::string host;
    int port;
    std::string state;
    // VDR video disk capacity, in MiB; negative means not reported.
    std::int64_t diskFreeMb = -1;
};
