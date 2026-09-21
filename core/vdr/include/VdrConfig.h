#pragma once

#include <string>

struct VdrConfig
{
    bool enabled;
    std::string mode;
    std::string host;
    int port;

    VdrConfig();
};
