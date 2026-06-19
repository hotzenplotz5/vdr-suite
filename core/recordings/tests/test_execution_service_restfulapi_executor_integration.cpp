#include "MockHttpClient.h"
#include "RecordingActionBackendPolicy.h"
#include "RecordingActionExecutionService.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cassert>
#include <memory>
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

RecordingActionBackendPolicy makeAllowedPolicy()
{
    RecordingActionBackendPolicyFactory factory;
    return factory.restfulApiMutationPolicy("local-vdr");
}
}

int main()
{
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body = "Recording moved!";
    httpClient.setResponse(response);

    RecordingActionBackendExecutorAdapterRegistry registry;

    auto executor =
        std::make_shared<RestfulApiRecordingActionExecutor>(
            "local-vdr",
            "restfulapi",
            makeConfig(),
            httpClient);

    registry.registerAdapter(executor);

    RecordingActionExecutionService service;

    const RecordingActionExecutionResult result =
        service.execute(
            makeMoveRequest(),
            registry,
            makeAllowedPolicy());

    assert(result.success);
    assert(result.type == RecordingActionType::Move);
    assert(result.backendId == "local-vdr");
    assert(result.recordingId == "Tagesschau/2026-06-17.20.00.10-0.rec");
    assert(result.message == "RESTfulAPI recording action request executed");
    assert(result.errors.empty());

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
