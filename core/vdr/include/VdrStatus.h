#pragma once

#include <string>

struct VdrStatus
{
    bool enabled;
    std::string mode;
    std::string host;
    int port;
    std::string state;
};
