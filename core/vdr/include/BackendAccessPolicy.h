#pragma once

#include "BackendRegistryService.h"

#include <string>
#include <vector>

struct BackendAccessDecision
{
    bool allowed = false;
    bool backendFound = false;
    bool readOnly = false;
    std::string backendId;
    std::string accessMode;
    std::string reason;
    std::vector<std::string> errors;
};

class BackendAccessPolicy
{
public:
    BackendAccessDecision canWriteToBackend(
        const BackendRegistryService& registryService,
        const std::string& backendId) const
    {
        BackendAccessDecision decision;
        decision.backendId = backendId;

        if (backendId.empty())
        {
            decision.reason = "backend id is required";
            decision.errors.push_back(decision.reason);
            return decision;
        }

        const auto backend = registryService.getBackend(backendId);

        if (!backend.has_value())
        {
            decision.reason = "backend not found";
            decision.errors.push_back(decision.reason);
            return decision;
        }

        decision.backendFound = true;
        decision.accessMode = backend->accessMode;
        decision.readOnly = backend->readOnly();

        if (!backend->enabled)
        {
            decision.reason = "backend is disabled";
            decision.errors.push_back(decision.reason);
            return decision;
        }

        if (backend->readOnly())
        {
            decision.reason = "backend is read-only";
            decision.errors.push_back(decision.reason);
            return decision;
        }

        decision.allowed = true;
        decision.reason = "backend write access allowed";
        return decision;
    }
};
