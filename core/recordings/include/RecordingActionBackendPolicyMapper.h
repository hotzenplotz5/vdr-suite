#pragma once

#include "BackendNode.h"
#include "RecordingActionBackendPolicy.h"

class RecordingActionBackendPolicyMapper
{
public:
    RecordingActionBackendPolicy fromBackendNode(
        const BackendNode& backend) const
    {
        RecordingActionBackendPolicy policy;

        policy.backendId = backend.backendId;
        policy.backendAvailable = backend.enabled && backend.online;

        if (backend.readOnly() || backend.backendType == "restfulapi-readonly")
        {
            policy.readOnly = true;
            policy.executionAllowed = false;
            policy.capabilities =
                policyFactory_.readOnlyPolicy(backend.backendId).capabilities;
            policy.permissions =
                policyFactory_.readOnlyPolicy(backend.backendId).permissions;
            return policy;
        }

        if (backend.backendType == "restfulapi")
        {
            policy =
                policyFactory_.restfulApiMutationPolicy(backend.backendId);
            policy.backendAvailable = backend.enabled && backend.online;
            return policy;
        }

        policy =
            policyFactory_.readOnlyPolicy(backend.backendId);
        policy.backendAvailable = backend.enabled && backend.online;
        return policy;
    }

private:
    RecordingActionBackendPolicyFactory policyFactory_;
};
