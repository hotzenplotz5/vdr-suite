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

RecordingActionJobPayload makeDeletePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = false;
    payload.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    return payload;
}

RecordingActionExecutionResult executeWithResponse(
    int statusCode,
    const std::string& body,
    MockHttpClient& httpClient)
{
    HttpResponse response;
    response.statusCode = statusCode;
    response.body = body;
    httpClient.setResponse(response);

    RestfulApiRecordingActionExecutor executor(
        "local-vdr",
        "restfulapi",
        makeConfig(),
        httpClient);

    return executor.execute(makeDeletePayload());
}
}

int main()
{
    {
        MockHttpClient httpClient;
        const RecordingActionExecutionResult result =
            executeWithResponse(
                200,
                "Recording deleted!",
                httpClient);

        assert(result.success);
        assert(result.message == "RESTfulAPI recording action request executed");
        assert(result.errors.empty());
        assert(httpClient.requestCount() == 1);
        assert(httpClient.lastRequest().method == "POST");
        assert(httpClient.lastRequest().url == "/recordings/delete.json");
        assert(httpClient.lastRequest().body.find("\"file\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    }

    {
        MockHttpClient httpClient;
        const RecordingActionExecutionResult result =
            executeWithResponse(
                404,
                "Recording not found!",
                httpClient);

        assert(!result.success);
        assert(result.message == "RESTfulAPI recording action request failed");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) == "RESTfulAPI returned HTTP status 404: Recording not found!");
        assert(httpClient.requestCount() == 1);
    }

    {
        MockHttpClient httpClient;
        const RecordingActionExecutionResult result =
            executeWithResponse(
                500,
                "",
                httpClient);

        assert(!result.success);
        assert(result.message == "RESTfulAPI recording action request failed");
        assert(result.errors.size() == 1);
        assert(result.errors.at(0) == "RESTfulAPI returned HTTP status 500");
        assert(httpClient.requestCount() == 1);
    }

    return 0;
}
