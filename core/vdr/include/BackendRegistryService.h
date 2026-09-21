#pragma once

#include "BackendNode.h"
#include "BackendRegistry.h"

#include <optional>
#include <string>
#include <vector>

class BackendRegistryService
{
public:
    explicit BackendRegistryService(BackendRegistry& registry);

    bool hasBackend(const std::string& backendId) const;
    std::optional<BackendNode> getBackend(const std::string& backendId) const;
    bool updateBackendCapabilities(
        const std::string& backendId,
        const VdrCapabilitySet& capabilities);
    std::vector<BackendNode> listBackends() const;
    std::optional<BackendNode> defaultBackend() const;

private:
    BackendRegistry& registry_;
};
