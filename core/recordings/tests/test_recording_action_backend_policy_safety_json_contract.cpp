#include "RecordingActionExecutionService.h"
#include "RecordingActionSafetyResultJsonSerializer.h"

#include <cassert>
#include <string>

int main()
{
    RecordingActionExecutionService service;
    RecordingActionBackendPolicyFactory factory;
    RecordingActionSafetyResultJsonSerializer serializer;

    {
        RecordingActionRequest request;
        request.backendId = "remote-house";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Move;
        request.dryRun = true;

        const RecordingActionBackendPolicy policy =
            factory.readOnlyPolicy("remote-house");

        const std::string json =
            serializer.serialize(
                service.evaluateSafety(request, policy));

        assert(json.find("\"canExecute\":false") != std::string::npos);
        assert(json.find("\"dryRun\":true") != std::string::npos);
        assert(json.find("\"readOnlyBlocked\":true") != std::string::npos);
        assert(json.find("\"backendUnavailable\":false") != std::string::npos);
        assert(json.find("\"reasons\":[\"backend_read_only\",\"permission_denied\"]") != std::string::npos);
        assert(json.find("recording action execution is blocked by read-only backend config") != std::string::npos);
        assert(json.find("missing permission: recording.permission.move") != std::string::npos);
    }

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Move;
        request.dryRun = false;

        const RecordingActionBackendPolicy policy =
            factory.restfulApiMutationPolicy("living-room");

        const std::string json =
            serializer.serialize(
                service.evaluateSafety(request, policy));

        assert(json.find("\"canExecute\":true") != std::string::npos);
        assert(json.find("\"dryRun\":false") != std::string::npos);
        assert(json.find("\"readOnlyBlocked\":false") != std::string::npos);
        assert(json.find("\"reasons\":[]") != std::string::npos);
        assert(json.find("\"blockers\":[]") != std::string::npos);
    }

    {
        RecordingActionRequest request;
        request.backendId = "offline-room";
        request.recordingId = "recording-3";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        RecordingActionBackendPolicy policy =
            factory.restfulApiMutationPolicy("offline-room");
        policy.backendAvailable = false;

        const std::string json =
            serializer.serialize(
                service.evaluateSafety(request, policy));

        assert(json.find("\"canExecute\":false") != std::string::npos);
        assert(json.find("\"backendUnavailable\":true") != std::string::npos);
        assert(json.find("\"reasons\":[\"backend_unavailable\"]") != std::string::npos);
        assert(json.find("recording action backend is unavailable") != std::string::npos);
    }

    return 0;
}
