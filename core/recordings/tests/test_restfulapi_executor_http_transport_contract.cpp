#include "MockHttpClient.h"
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
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK";
    httpClient.setResponse(response);

    RestfulApiRecordingActionExecutor executor(
        "local-vdr",
        "restfulapi",
        makeConfig(),
        httpClient);

    const RecordingActionExecutionResult result =
        executor.execute(makeMovePayload());

    assert(result.success);
    assert(result.type == RecordingActionType::Move);
    assert(result.backendId == "local-vdr");
    assert(result.recordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(result.message == "RESTfulAPI recording action request executed");

    assert(httpClient.requestCount() == 1);

    const HttpRequest& request =
        httpClient.lastRequest();

    assert(request.method == "POST");
    assert(request.url == "/recordings/move.json");
    assert(request.headers.at("Accept") == "application/json");
    assert(request.headers.at("Content-Type") == "application/json");
    assert(request.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    assert(request.body.find("\"target\":\"Archiv~Tagesschau~Tagesschau\"") != std::string::npos);
    assert(request.body.find("\"copy_only\":false") != std::string::npos);
    assert(request.body.find("Archiv/Tagesschau") == std::string::npos);

    return 0;
}
