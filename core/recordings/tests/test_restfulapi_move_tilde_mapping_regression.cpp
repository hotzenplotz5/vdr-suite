#include "RestfulApiRecordingActionRequestBuilder.h"

#include <cassert>
#include <string>

int main()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "local-vdr";
    config.host = "127.0.0.1";
    config.port = 8002;
    config.basePath = "";
    config.readOnly = true;
    config.allowExecution = false;

    RecordingActionJobPayload payload;
    payload.backendId = "local-vdr";
    payload.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.type = RecordingActionType::Move;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    payload.parameters["targetPath"] =
        "Archiv/Tagesschau";

    RestfulApiRecordingActionRequestBuilder builder;

    const HttpRequest request =
        builder.buildMoveRequest(config, payload);

    assert(request.method == "POST");
    assert(request.url == "/recordings/move.json");

    assert(request.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    assert(request.body.find("\"target\":\"Archiv~Tagesschau~Tagesschau\"") != std::string::npos);
    assert(request.body.find("\"copy_only\":false") != std::string::npos);

    assert(request.body.find("Archiv/Tagesschau") == std::string::npos);
    assert(request.body.find("Archiv~~Tagesschau") == std::string::npos);
    assert(request.body.find("/api/") == std::string::npos);

    return 0;
}
