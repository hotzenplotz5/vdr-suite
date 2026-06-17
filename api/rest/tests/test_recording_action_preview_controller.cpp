#include "RecordingActionPreviewController.h"

#include "RecordingActionValidationRequestParser.h"

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
    RecordingActionRequestPreviewService previewService;
    RecordingActionRequestPreviewResultJsonSerializer jsonSerializer;
    RecordingActionValidationRequestParser requestParser;

    RecordingActionPreviewController controller(
        previewService,
        jsonSerializer,
        requestParser);

    const ApiResponse response =
        controller.previewBody(
            "{\"action\":\"MOVE\","
            "\"backendId\":\"local-vdr\","
            "\"recordingId\":\"Tagesschau/2026-06-17.20.00.10-0.rec\","
            "\"recordingPath\":\"Tagesschau/2026-06-17.20.00.10-0.rec\","
            "\"targetPath\":\"Archiv/Tagesschau\","
            "\"dryRun\":true}",
            makeConfig());

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");
    assert(response.body.find("\"success\":true") != std::string::npos);
    assert(response.body.find("\"type\":\"MOVE\"") != std::string::npos);
    assert(response.body.find("\"backendId\":\"local-vdr\"") != std::string::npos);
    assert(response.body.find("\"method\":\"POST\"") != std::string::npos);
    assert(response.body.find("\"url\":\"/recordings/move.json\"") != std::string::npos);
    assert(response.body.find("\\\"target\\\":\\\"Archiv~Tagesschau\\\"") != std::string::npos);
    assert(response.body.find("Archiv/Tagesschau") == std::string::npos);
    assert(response.body.find("/api/") == std::string::npos);

    return 0;
}
