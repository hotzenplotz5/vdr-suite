#include "MockHttpClient.h"
#include "RecordingActionRequestPreviewService.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cassert>
#include <string>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "local-vdr";
    config.host = "127.0.0.1";
    config.port = 8002;
    config.basePath = "";
    config.readOnly = false;
    config.allowExecution = true;
    return config;
}

RecordingActionRequest makeMoveRequest()
{
    RecordingActionRequest request;
    request.backendId = "local-vdr";
    request.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.type = RecordingActionType::Move;
    request.dryRun = false;
    request.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.parameters["targetPath"] =
        "Archiv/Tagesschau";
    return request;
}

RecordingActionJobPayload makeMovePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.type = RecordingActionType::Move;
    payload.dryRun = false;
    payload.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.parameters["targetPath"] =
        "Archiv/Tagesschau";
    return payload;
}
}

int main()
{
    const RestfulApiRecordingActionBackendConfig config =
        makeConfig();

    RecordingActionRequestPreviewService previewService;

    const RecordingActionRequestPreviewResult preview =
        previewService.previewRestfulApiRequest(
            makeMoveRequest(),
            config);

    assert(preview.success);
    assert(preview.method == "POST");
    assert(preview.url == "/recordings/move.json");

    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body = "Recording moved!";
    httpClient.setResponse(response);

    RestfulApiRecordingActionExecutor executor(
        "local-vdr",
        "restfulapi",
        config,
        httpClient);

    const RecordingActionExecutionResult execution =
        executor.execute(makeMovePayload());

    assert(execution.success);
    assert(httpClient.requestCount() == 1);

    const HttpRequest& executionRequest =
        httpClient.lastRequest();

    assert(executionRequest.method == preview.method);
    assert(executionRequest.url == preview.url);
    assert(executionRequest.headers == preview.headers);
    assert(executionRequest.body == preview.body);

    assert(preview.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    assert(preview.body.find("\"target\":\"Archiv~Tagesschau\"") != std::string::npos);
    assert(preview.body.find("\"copy_only\":false") != std::string::npos);
    assert(preview.body.find("Archiv/Tagesschau") == std::string::npos);

    return 0;
}
