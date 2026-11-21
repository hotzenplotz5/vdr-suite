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

RecordingActionJobPayload makeBasePayload(
    RecordingActionType type)
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    payload.type = type;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Mystery/The_Village_-_Das_Dorf/2010-10-31.02.29.10-0.rec";
    return payload;
}
}

int main()
{
    RestfulApiRecordingActionRequestBuilder builder;
    const RestfulApiRecordingActionBackendConfig config = makeConfig();

    {
        const RecordingActionJobPayload payload =
            makeBasePayload(RecordingActionType::Delete);

        const HttpRequest request =
            builder.buildDeleteRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/delete.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");
        assert(request.body.find("\"file\"") != std::string::npos);
        assert(request.body.find(payload.recordingId) != std::string::npos);
    }

    {
        RecordingActionJobPayload payload =
            makeBasePayload(RecordingActionType::Move);
        payload.parameters["targetPath"] = "Archive/Mystery";

        const HttpRequest request =
            builder.buildMoveRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/move.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");
        assert(request.body.find("\"source\"") != std::string::npos);
        assert(request.body.find("\"target\":\"Archive~Mystery~The_Village_-_Das_Dorf\"") != std::string::npos);
        assert(request.body.find("\"copy_only\":false") != std::string::npos);
    }

    {
        RecordingActionJobPayload payload =
            makeBasePayload(RecordingActionType::Rename);
        payload.parameters["newName"] = "Mystery/The Village - Das Dorf HD";

        const HttpRequest request =
            builder.buildRenameRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/move.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");
        assert(request.body.find("\"source\"") != std::string::npos);
        assert(request.body.find("\"target\":\"Mystery~The_Village_-_Das_Dorf_HD\"") != std::string::npos);
        assert(request.body.find("\"copy_only\":false") != std::string::npos);
    }

    return 0;
}
