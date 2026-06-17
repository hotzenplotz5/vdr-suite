#include "MockHttpClient.h"
#include "RestfulApiRecordingActionBackendExecutorAdapter.h"

#include <cassert>
#include <string>

namespace
{
RestfulApiRecordingActionBackendConfig makeSmokeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "local-vdr";
    config.host = "127.0.0.1";
    config.port = 8002;
    config.basePath = "/api";
    config.readOnly = false;
    config.allowExecution = false;
    return config;
}

RecordingActionJobPayload makeDryRunDeletePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    return payload;
}
}

int main()
{
    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body = "Dry-run transport smoke accepted";
    httpClient.setResponse(response);

    RestfulApiRecordingActionBackendExecutorAdapter adapter(
        makeSmokeConfig(),
        httpClient);

    const RecordingActionExecutionResult result =
        adapter.execute(makeDryRunDeletePayload());

    assert(result.success);
    assert(result.backendId == "local-vdr");
    assert(result.type == RecordingActionType::Delete);
    assert(result.message == "restfulapi backend executor request accepted");

    assert(httpClient.requestCount() == 1);

    const HttpRequest& request =
        httpClient.lastRequest();

    assert(request.method == "POST");
    assert(request.url == "/api/recordings/delete.json");
    assert(request.body.find("Movies/Tatort/2026-06-16.20.15.1-0.rec")
           != std::string::npos);

    return 0;
}
