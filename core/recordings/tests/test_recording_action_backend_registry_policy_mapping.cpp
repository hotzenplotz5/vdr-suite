#include "RecordingActionBackendPolicyMapper.h"

#include <cassert>

int main()
{
    RecordingActionBackendPolicyMapper mapper;

    {
        BackendNode backend;
        backend.backendId = "living-room";
        backend.backendType = "restfulapi";
        backend.enabled = true;
        backend.online = true;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "living-room");
        assert(policy.backendAvailable);
        assert(!policy.readOnly);
        assert(policy.executionAllowed);
        assert(policy.capabilities.contains("recording.action.move"));
        assert(policy.permissions.contains("recording.permission.move"));
        assert(policy.permissions.contains("recording.permission.delete"));
    }

    {
        BackendNode backend;
        backend.backendId = "remote-house-access-mode";
        backend.backendType = "restfulapi";
        backend.accessMode = "read-only";
        backend.enabled = true;
        backend.online = true;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "remote-house-access-mode");
        assert(policy.backendAvailable);
        assert(policy.readOnly);
        assert(!policy.executionAllowed);
        assert(policy.capabilities.contains("recording.action.move"));
        assert(policy.permissions.contains("recording.permission.view"));
        assert(!policy.permissions.contains("recording.permission.move"));
        assert(!policy.permissions.contains("recording.permission.delete"));
    }

    {
        BackendNode backend;
        backend.backendId = "remote-house";
        backend.backendType = "restfulapi-readonly";
        backend.enabled = true;
        backend.online = true;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "remote-house");
        assert(policy.backendAvailable);
        assert(policy.readOnly);
        assert(!policy.executionAllowed);
        assert(policy.capabilities.contains("recording.action.move"));
        assert(policy.permissions.contains("recording.permission.view"));
        assert(!policy.permissions.contains("recording.permission.move"));
        assert(!policy.permissions.contains("recording.permission.delete"));
    }

    {
        BackendNode backend;
        backend.backendId = "offline-room";
        backend.backendType = "restfulapi";
        backend.enabled = true;
        backend.online = false;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "offline-room");
        assert(!policy.backendAvailable);
        assert(!policy.readOnly);
        assert(policy.executionAllowed);
    }

    {
        BackendNode backend;
        backend.backendId = "disabled-room";
        backend.backendType = "restfulapi";
        backend.enabled = false;
        backend.online = true;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "disabled-room");
        assert(!policy.backendAvailable);
        assert(!policy.readOnly);
        assert(policy.executionAllowed);
    }

    {
        BackendNode backend;
        backend.backendId = "unknown-backend";
        backend.backendType = "unknown";
        backend.enabled = true;
        backend.online = true;

        const RecordingActionBackendPolicy policy =
            mapper.fromBackendNode(backend);

        assert(policy.backendId == "unknown-backend");
        assert(policy.backendAvailable);
        assert(policy.readOnly);
        assert(!policy.executionAllowed);
        assert(policy.permissions.contains("recording.permission.view"));
        assert(!policy.permissions.contains("recording.permission.move"));
    }

    return 0;
}
