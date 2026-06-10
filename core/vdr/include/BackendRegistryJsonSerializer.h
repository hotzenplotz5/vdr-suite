#pragma once

#include "BackendNode.h"

#include <string>
#include <vector>

class BackendRegistryJsonSerializer
{
public:
    std::string serializeBackend(
        const BackendNode& backend) const;

    std::string serializeBackends(
        const std::vector<BackendNode>& backends) const;
};
