#include "MockHttpClient.h"
#include "RecordingActionExecutionService.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <memory>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig(
    const std::string& backendId)
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = backendId;
    config.basePath = "/api";
    config.readOnly = false;
    config.allowExecution = false;
    return config;
}
}

int main()
{
    MockHttpClient httpClient;
    RecordingActionExecutionService service;
    RecordingActionBackendExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
            makeConfig("living-room"),
            httpClient));

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Delete;
        request.dryRun = true;

        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, context, registry);

        assert(result.canExecute);
        assert(result.dryRun);
        assert(!result.missingCapability);
        assert(result.blockers.empty());
        assert(result.warnings.size() == 1);
        assert(result.warnings.at(0) == "dry-run only");
    }

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-2";
        request.type = RecordingActionType::Cut;
        request.dryRun = false;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, context, registry);

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.blockers.size() == 2);
        assert(result.blockers.at(0) ==
               "recording action capability is missing");
        assert(result.blockers.at(1) ==
               "missing capability: recording.action.cut");
    }

    {
        RecordingActionRequest request;
        request.backendId = "missing-backend";
        request.recordingId = "recording-3";
        request.type = RecordingActionType::Delete;
        request.dryRun = false;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, context, registry);

        assert(!result.canExecute);
        assert(result.missingCapability);
        assert(result.blockers.size() == 2);
        assert(result.blockers.at(0) ==
               "recording action capability is missing");
        assert(result.blockers.at(1) ==
               "missing capability: recording.action.delete");
    }

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-4";
        request.type = RecordingActionType::Move;
        request.dryRun = false;

        RecordingActionSafetyContext context;
        context.dryRun = false;
        context.executionAllowed = true;
        context.backendReadOnly = true;

        const RecordingActionSafetyResult result =
            service.evaluateSafety(request, context, registry);

        assert(!result.canExecute);
        assert(result.readOnlyBlocked);
        assert(!result.missingCapability);
        assert(result.blockers.size() == 1);
        assert(result.blockers.at(0) ==
               "recording action execution is blocked by read-only backend config");
    }

    return 0;
}
