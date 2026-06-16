#include "RecordingActionExecutionController.h"

#include "MockHttpClient.h"
#include "RecordingActionExecutionResultJsonSerializer.h"
#include "RecordingActionExecutionService.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <memory>
#include <string>

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
    RecordingActionExecutionService executionService;
    RecordingActionExecutionResultJsonSerializer executionSerializer;
    RecordingActionBackendExecutorAdapterRegistry registry;

    registry.registerAdapter(
        std::make_shared<RestfulApiRecordingActionBackendExecutorAdapter>(
            makeConfig("living-room"),
            httpClient));

    RecordingActionExecutionController controller(
        executionService,
        executionSerializer,
        registry);

    {
        RecordingActionRequest request;
        request.backendId = "living-room";
        request.recordingId = "recording-1";
        request.type = RecordingActionType::Delete;
        request.dryRun = true;

        RecordingActionSafetyContext context;
        context.dryRun = true;
        context.executionAllowed = false;

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body.find("\"canExecute\":true") != std::string::npos);
        assert(response.body.find("\"dryRun\":true") != std::string::npos);
        assert(response.body.find("\"missingCapability\":false") != std::string::npos);
        assert(response.body.find("\"blockers\":[]") != std::string::npos);
        assert(response.body.find("\"warnings\":[\"dry-run only\"]") != std::string::npos);
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

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(response.body.find("\"canExecute\":false") != std::string::npos);
        assert(response.body.find("\"missingCapability\":true") != std::string::npos);
        assert(response.body.find("missing capability: recording.action.cut") != std::string::npos);
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

        const ApiResponse response =
            controller.safety(request, context);

        assert(response.statusCode == 200);
        assert(response.body.find("\"canExecute\":false") != std::string::npos);
        assert(response.body.find("\"missingCapability\":true") != std::string::npos);
        assert(response.body.find("missing capability: recording.action.delete") != std::string::npos);
    }

    return 0;
}
