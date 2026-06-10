#pragma once

#include "BackendNode.h"

#include <optional>
#include <string>
#include <vector>

class BackendRegistry
{
public:
    void addBackend(const BackendNode& backend);
    bool hasBackend(const std::string& backendId) const;
    std::optional<BackendNode> getBackend(const std::string& backendId) const;
    std::vector<BackendNode> listBackends() const;

private:
    std::vector<BackendNode> backends_;
};
