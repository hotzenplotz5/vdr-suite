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

RecordingActionJobPayload makePayload(
    RecordingActionType type)
{
    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.type = type;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    return payload;
}
}

int main()
{
    RestfulApiRecordingActionRequestBuilder builder;
    const RestfulApiRecordingActionBackendConfig config =
        makeConfig();

    {
        RecordingActionJobPayload payload =
            makePayload(RecordingActionType::Move);
        payload.parameters["targetPath"] =
            "Archiv/Tagesschau";

        const HttpRequest request =
            builder.buildMoveRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/move.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");

        assert(request.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
        assert(request.body.find("\"target\":\"Archiv~Tagesschau\"") != std::string::npos);
        assert(request.body.find("\"copy_only\":false") != std::string::npos);

        assert(request.body.find("\"file\"") == std::string::npos);
        assert(request.body.find("Archiv/Tagesschau") == std::string::npos);
        assert(request.body.find("Archiv~~Tagesschau") == std::string::npos);
    }

    {
        RecordingActionJobPayload payload =
            makePayload(RecordingActionType::Rename);
        payload.parameters["newName"] =
            "Archiv/Tagesschau kurz";

        const HttpRequest request =
            builder.buildRenameRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/move.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");

        assert(request.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
        assert(request.body.find("\"target\":\"Archiv~Tagesschau kurz\"") != std::string::npos);
        assert(request.body.find("\"copy_only\":false") != std::string::npos);

        assert(request.body.find("\"file\"") == std::string::npos);
        assert(request.body.find("Archiv/Tagesschau kurz") == std::string::npos);
    }

    {
        const RecordingActionJobPayload payload =
            makePayload(RecordingActionType::Delete);

        const HttpRequest request =
            builder.buildDeleteRequest(config, payload);

        assert(request.method == "POST");
        assert(request.url == "/recordings/delete.json");
        assert(request.headers.at("Accept") == "application/json");
        assert(request.headers.at("Content-Type") == "application/json");

        assert(request.body.find("\"file\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);

        assert(request.body.find("\"source\"") == std::string::npos);
        assert(request.body.find("\"target\"") == std::string::npos);
        assert(request.body.find("\"copy_only\"") == std::string::npos);
    }

    return 0;
}
