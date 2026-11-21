#include "MockHttpClient.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cassert>
#include <string>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "default";
    config.basePath = "";
    config.readOnly = false;
    config.allowExecution = true;
    return config;
}

RecordingActionJobPayload makeDeletePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "default";
    payload.recordingId = "missing.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = false;
    payload.parameters["recordingPath"] = "missing.rec";
    return payload;
}
}

int main()
{
    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 404;
        response.body = "";
        httpClient.setResponse(response);

        RestfulApiRecordingActionExecutor executor(
            "default",
            "restfulapi",
            makeConfig(),
            httpClient);

        const RecordingActionExecutionResult result =
            executor.execute(makeDeletePayload());

        assert(!result.success);
        assert(result.message == "RESTfulAPI recording action request failed");
        assert(result.errors.size() == 1);
        assert(result.errors[0] == "RESTfulAPI returned HTTP status 404");
    }

    {
        MockHttpClient httpClient;
        HttpResponse response;
        response.statusCode = 500;
        response.body = "Broken";
        httpClient.setResponse(response);

        RestfulApiRecordingActionExecutor executor(
            "default",
            "restfulapi",
            makeConfig(),
            httpClient);

        const RecordingActionExecutionResult result =
            executor.execute(makeDeletePayload());

        assert(!result.success);
        assert(result.message == "RESTfulAPI recording action request failed");
        assert(result.errors.size() == 1);
        assert(result.errors[0] == "RESTfulAPI returned HTTP status 500: Broken");
    }

    return 0;
}
