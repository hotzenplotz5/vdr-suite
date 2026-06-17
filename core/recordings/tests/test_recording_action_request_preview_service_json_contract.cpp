#include "RecordingActionRequestPreviewResultJsonSerializer.h"
#include "RecordingActionRequestPreviewService.h"

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
}

int main()
{
    RecordingActionRequest request;
    request.backendId = "local-vdr";
    request.recordingId =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.type = RecordingActionType::Move;
    request.dryRun = true;
    request.parameters["recordingPath"] =
        "Tagesschau/2026-06-17.20.00.10-0.rec";
    request.parameters["targetPath"] =
        "Archiv/Tagesschau";

    RecordingActionRequestPreviewService service;
    RecordingActionRequestPreviewResultJsonSerializer serializer;

    const RecordingActionRequestPreviewResult result =
        service.previewRestfulApiRequest(request, makeConfig());

    const std::string json =
        serializer.serialize(result);

    assert(json.find("\"success\":true") != std::string::npos);
    assert(json.find("\"type\":\"MOVE\"") != std::string::npos);
    assert(json.find("\"backendId\":\"local-vdr\"") != std::string::npos);
    assert(json.find("\"recordingId\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
    assert(json.find("\"method\":\"POST\"") != std::string::npos);
    assert(json.find("\"url\":\"/recordings/move.json\"") != std::string::npos);
    assert(json.find("\"Accept\":\"application/json\"") != std::string::npos);
    assert(json.find("\"Content-Type\":\"application/json\"") != std::string::npos);
    assert(json.find("\\\"source\\\":\\\"Tagesschau/2026-06-17.20.00.10-0.rec\\\"") != std::string::npos);
    assert(json.find("\\\"target\\\":\\\"Archiv~Tagesschau\\\"") != std::string::npos);
    assert(json.find("Archiv/Tagesschau") == std::string::npos);
    assert(json.find("/api/") == std::string::npos);

    return 0;
}
