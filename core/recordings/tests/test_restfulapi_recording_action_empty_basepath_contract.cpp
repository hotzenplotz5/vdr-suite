#include "RestfulApiRecordingActionRequestBuilder.h"

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
    config.readOnly = true;
    config.allowExecution = false;
    return config;
}

RecordingActionJobPayload makeDeletePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId = "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    payload.type = RecordingActionType::Delete;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    return payload;
}

RecordingActionJobPayload makeMovePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId = "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    payload.type = RecordingActionType::Move;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    payload.parameters["targetPath"] = "Archive/Mystery";
    return payload;
}
}

int main()
{
    RestfulApiRecordingActionRequestBuilder builder;
    const RestfulApiRecordingActionBackendConfig config = makeConfig();

    {
        const HttpRequest request =
            builder.buildDeleteRequest(config, makeDeletePayload());

        assert(request.method == "POST");
        assert(request.url == "/recordings/delete.json");
        assert(request.url.find("/api/") == std::string::npos);
        assert(request.body.find("\"file\"") != std::string::npos);
    }

    {
        const HttpRequest request =
            builder.buildMoveRequest(config, makeMovePayload());

        assert(request.method == "POST");
        assert(request.url == "/recordings/move.json");
        assert(request.url.find("/api/") == std::string::npos);
        assert(request.body.find("\"source\"") != std::string::npos);
        assert(request.body.find("\"target\":\"Archive~Mystery~The_Village_-_Das_Dorf\"") != std::string::npos);
    }

    return 0;
}
