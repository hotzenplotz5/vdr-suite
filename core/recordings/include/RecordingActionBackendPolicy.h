#pragma once

#include "RecordingActionCapabilityContract.h"
#include "RecordingActionPermissionContract.h"

#include <string>

struct RecordingActionBackendPolicy
{
    std::string backendId;
    bool backendAvailable = true;
    bool readOnly = false;
    bool executionAllowed = false;

    RecordingActionCapabilitySet capabilities;
    RecordingActionPermissionSet permissions;
};

class RecordingActionBackendPolicyFactory
{
public:
    RecordingActionBackendPolicy readOnlyPolicy(
        const std::string& backendId) const
    {
        RecordingActionBackendPolicy policy;
        policy.backendId = backendId;
        policy.readOnly = true;
        policy.executionAllowed = false;
        policy.capabilities =
            capabilityContract_.restfulApiDefaultCapabilities();
        policy.permissions =
            permissionContract_.readOnlyPermissions();
        return policy;
    }

    RecordingActionBackendPolicy restfulApiMutationPolicy(
        const std::string& backendId) const
    {
        RecordingActionBackendPolicy policy;
        policy.backendId = backendId;
        policy.readOnly = false;
        policy.executionAllowed = true;
        policy.capabilities =
            capabilityContract_.restfulApiDefaultCapabilities();
        policy.permissions =
            permissionContract_.restfulApiMutationPermissions();
        return policy;
    }

private:
    RecordingActionCapabilityContract capabilityContract_;
    RecordingActionPermissionContract permissionContract_;
};
