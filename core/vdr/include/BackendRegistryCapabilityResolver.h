#pragma once

#include "BackendRegistryService.h"
#include "CapabilityResolver.h"
#include "CapabilityState.h"
#include "ICapabilityResolver.h"

#include <string>
#include <utility>

class BackendRegistryCapabilityResolver : public ICapabilityResolver
{
public:
    BackendRegistryCapabilityResolver(
        BackendRegistryService& backendRegistryService,
        std::string backendId)
        : backendRegistryService_(backendRegistryService),
          backendId_(std::move(backendId))
    {
    }

    bool supports(
        const std::string& capability) const override
    {
        return state(capability).availableNow();
    }

    CapabilityState state(
        const std::string& capability) const override
    {
        const auto backend =
            backendRegistryService_.getBackend(backendId_);

        if (!backend.has_value())
        {
            return CapabilityState::unsupported(
                capability,
                "backend not found: " + backendId_);
        }

        CapabilityResolver resolver(
            backend->capabilities);

        return resolver.state(capability);
    }

private:
    BackendRegistryService& backendRegistryService_;
    std::string backendId_;
};
