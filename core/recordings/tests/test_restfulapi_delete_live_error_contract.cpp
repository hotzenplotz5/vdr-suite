#include "BasicHttpClient.h"
#include "RestfulApiRecordingActionExecutor.h"

#include <cassert>
#include <string>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "real-vdr";
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
    payload.backendId = "real-vdr";
    payload.recordingId = "__vdr_suite_nonexistent_source__.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = false;
    payload.parameters["recordingPath"] =
        "__vdr_suite_nonexistent_source__.rec";
    return payload;
}
}

int main()
{
    BasicHttpClient httpClient("127.0.0.1", 8002);

    RestfulApiRecordingActionExecutor executor(
        "real-vdr",
        "restfulapi",
        makeConfig(),
        httpClient);

    const RecordingActionExecutionResult result =
        executor.execute(makeDeletePayload());

    assert(!result.success);
    assert(result.type == RecordingActionType::Delete);
    assert(result.backendId == "real-vdr");
    assert(result.recordingId == "__vdr_suite_nonexistent_source__.rec");
    assert(result.message == "RESTfulAPI recording action request failed");
    assert(result.errors.size() == 1);
    assert(result.errors[0].find("404") != std::string::npos);

    return 0;
}
