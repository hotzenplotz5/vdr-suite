#include "RecordingActionBackendPolicyProvider.h"

#include <cassert>

int main()
{
    BackendRegistry registry;

    BackendNode livingRoom;
    livingRoom.backendId = "living-room";
    livingRoom.backendType = "restfulapi";
    livingRoom.enabled = true;
    livingRoom.online = true;
    registry.addBackend(livingRoom);

    BackendNode remoteHouse;
    remoteHouse.backendId = "remote-house";
    remoteHouse.backendType = "restfulapi-readonly";
    remoteHouse.enabled = true;
    remoteHouse.online = true;
    registry.addBackend(remoteHouse);

    RecordingActionBackendPolicyProvider provider;

    {
        const RecordingActionBackendPolicyLookupResult lookup =
            provider.policyForBackend(registry, "living-room");

        assert(lookup.found);
        assert(lookup.message ==
               "recording action backend policy resolved");
        assert(lookup.policy.backendId == "living-room");
        assert(lookup.policy.backendAvailable);
        assert(!lookup.policy.readOnly);
        assert(lookup.policy.executionAllowed);
        assert(lookup.policy.permissions.contains(
            "recording.permission.move"));
    }

    {
        const RecordingActionBackendPolicyLookupResult lookup =
            provider.policyForBackend(registry, "remote-house");

        assert(lookup.found);
        assert(lookup.policy.backendId == "remote-house");
        assert(lookup.policy.backendAvailable);
        assert(lookup.policy.readOnly);
        assert(!lookup.policy.executionAllowed);
        assert(lookup.policy.permissions.contains(
            "recording.permission.view"));
        assert(!lookup.policy.permissions.contains(
            "recording.permission.move"));
    }

    {
        const RecordingActionBackendPolicyLookupResult lookup =
            provider.policyForBackend(registry, "missing-room");

        assert(!lookup.found);
        assert(lookup.message ==
               "recording action backend policy not found");
        assert(lookup.policy.backendId == "missing-room");
        assert(!lookup.policy.backendAvailable);
        assert(lookup.policy.readOnly);
        assert(!lookup.policy.executionAllowed);
    }

    return 0;
}
