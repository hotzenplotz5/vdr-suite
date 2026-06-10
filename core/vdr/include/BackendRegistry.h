#pragma once

#include "BackendNode.h"

#include <string>
#include <vector>

class BackendRegistry
{
public:
    void addBackend(const BackendNode& backend);
    bool hasBackend(const std::string& backendId) const;

private:
    std::vector<BackendNode> backends_;
};
