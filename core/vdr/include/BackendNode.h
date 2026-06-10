#pragma once

#include <string>

struct BackendNode
{
    std::string backendId = "default";
    std::string backendName = "Default VDR";
    std::string backendType = "vdr";
    bool enabled = true;
    bool online = false;
};
