#include "RecordingActionBackendPolicy.h"

#include <cassert>

int main()
{
    RecordingActionBackendPolicyFactory factory;

    {
        const RecordingActionBackendPolicy policy =
            factory.readOnlyPolicy("remote-house");

        assert(policy.backendId == "remote-house");
        assert(policy.backendAvailable);
        assert(policy.readOnly);
        assert(!policy.executionAllowed);

        assert(policy.capabilities.contains("recording.action.move"));
        assert(policy.capabilities.contains("recording.action.rename"));
        assert(policy.capabilities.contains("recording.action.delete"));

        assert(policy.permissions.contains("recording.permission.view"));
        assert(!policy.permissions.contains("recording.permission.move"));
        assert(!policy.permissions.contains("recording.permission.delete"));
    }

    {
        const RecordingActionBackendPolicy policy =
            factory.restfulApiMutationPolicy("living-room");

        assert(policy.backendId == "living-room");
        assert(policy.backendAvailable);
        assert(!policy.readOnly);
        assert(policy.executionAllowed);

        assert(policy.capabilities.contains("recording.action.move"));
        assert(policy.capabilities.contains("recording.action.rename"));
        assert(policy.capabilities.contains("recording.action.delete"));

        assert(policy.permissions.contains("recording.permission.view"));
        assert(policy.permissions.contains("recording.permission.move"));
        assert(policy.permissions.contains("recording.permission.rename"));
        assert(policy.permissions.contains("recording.permission.delete"));
    }

    return 0;
}
