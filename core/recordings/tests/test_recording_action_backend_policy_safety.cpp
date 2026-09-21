#include "RecordingActionExecutionService.h"

#include <cassert>

int main()
{
    RecordingActionExecutionService service;
    RecordingActionBackendPolicyFactory factory;

    {
        RecordingActionRequest request;
        request.backendId = "remote-house";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Move;
        request.dryRun = true;

        const RecordingActionBackendPolicy policy =
            factory.readOnlyPolicy("remote-house");

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, policy);

        assert(!result.canExecute);
        assert(result.dryRun);
        assert(result.readOnlyBlocked);
        assert(!result.missingCapability);
        assert(result.reasons.size() == 2);
        assert(result.reasons.at(0) == RecordingActionSafetyReason::BackendReadOnly);
        assert(result.reasons.at(1) == RecordingActionSafetyReason::PermissionDenied);
        assert(result.blockers.at(0) ==
               "recording action execution is blocked by read-only backend config");
        assert(result.blockers.at(1) ==
               "recording action permission is denied");
        assert(result.blockers.at(2) ==
               "missing permission: recording.permission.move");
    }

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Move;
        request.dryRun = false;

        const RecordingActionBackendPolicy policy =
            factory.restfulApiMutationPolicy("living-room");

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, policy);

        assert(result.canExecute);
        assert(!result.dryRun);
        assert(!result.readOnlyBlocked);
        assert(!result.missingCapability);
        assert(result.reasons.empty());
        assert(result.blockers.empty());
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

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, policy);

        assert(!result.canExecute);
        assert(!result.dryRun);
        assert(result.backendUnavailable);
        assert(result.reasons.size() == 1);
        assert(result.reasons.at(0) ==
               RecordingActionSafetyReason::BackendUnavailable);
        assert(result.blockers.at(0) ==
               "recording action backend is unavailable");
    }

    return 0;
}
