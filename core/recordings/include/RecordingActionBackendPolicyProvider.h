#pragma once

#include "BackendRegistry.h"
#include "RecordingActionBackendPolicyMapper.h"

#include <string>

struct RecordingActionBackendPolicyLookupResult
{
    bool found = false;
    RecordingActionBackendPolicy policy;
    std::string message;
};

class RecordingActionBackendPolicyProvider
{
public:
    RecordingActionBackendPolicyLookupResult policyForBackend(
        const BackendRegistry& registry,
        const std::string& backendId) const
    {
        RecordingActionBackendPolicyLookupResult result;

        const auto backend = registry.getBackend(backendId);
        if (!backend.has_value())
        {
            result.found = false;
            result.policy.backendId = backendId;
            result.policy.backendAvailable = false;
            result.policy.readOnly = true;
            result.policy.executionAllowed = false;
            result.message =
                "recording action backend policy not found";
            return result;
        }

        result.found = true;
        result.policy = mapper_.fromBackendNode(backend.value());
        result.message =
            "recording action backend policy resolved";
        return result;
    }

private:
    RecordingActionBackendPolicyMapper mapper_;
};
