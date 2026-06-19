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
    RecordingActionRequestPreviewService service;
    const RestfulApiRecordingActionBackendConfig config = makeConfig();

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

        const RecordingActionRequestPreviewResult result =
            service.previewRestfulApiRequest(request, config);

        assert(result.success);
        assert(result.backendId == "local-vdr");
        assert(result.recordingId == request.recordingId);
        assert(result.type == RecordingActionType::Move);
        assert(result.method == "POST");
        assert(result.url == "/recordings/move.json");
        assert(result.headers.at("Accept") == "application/json");
        assert(result.headers.at("Content-Type") == "application/json");
        assert(result.body.find("\"source\":\"Tagesschau/2026-06-17.20.00.10-0.rec\"") != std::string::npos);
        assert(result.body.find("\"target\":\"Archiv~Tagesschau~Tagesschau\"") != std::string::npos);
        assert(result.body.find("\"copy_only\":false") != std::string::npos);
    }

    {
        RecordingActionRequest request;
        request.backendId = "local-vdr";
        request.recordingId =
            "Tagesschau/2026-06-17.20.00.10-0.rec";
        request.type = RecordingActionType::MetadataRefresh;
        request.dryRun = true;

        const RecordingActionRequestPreviewResult result =
            service.previewRestfulApiRequest(request, config);

        assert(!result.success);
        assert(result.message == "recording action request preview action not supported");
        assert(!result.errors.empty());
    }

    return 0;
}
