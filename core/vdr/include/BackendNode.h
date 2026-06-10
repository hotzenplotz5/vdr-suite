#pragma once

#include "VdrCapabilitySet.h"
#include "VdrConfig.h"

#include <string>

struct BackendNode
{
    std::string backendId = "default";
    std::string backendName = "Default VDR";
    std::string backendType = "vdr";
    VdrConfig connection;
    VdrCapabilitySet capabilities;
    bool enabled = true;
    bool online = false;
};
