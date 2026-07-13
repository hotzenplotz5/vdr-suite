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
    payload.recordingId = "7983";
    payload.type = RecordingActionType::Move;
    payload.dryRun = true;
    payload.parameters["recordingPath"] =
        "/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec";
    payload.parameters["backendNativeId"] =
        "/srv/vdr/video/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec";
    payload.parameters["recordingTitle"] =
        "Ghibli/Die letzten Glühwürmchen (1988)";
    payload.parameters["targetPath"] =
        "__vdr_suite_move_probe__";

    RestfulApiRecordingActionRequestBuilder builder;

    const HttpRequest request =
        builder.buildMoveRequest(config, payload);

    assert(request.method == "POST");
    assert(request.url == "/recordings/move.json");

    assert(request.body.find(
        "\"source\":\"/srv/vdr/video/Recordings_on_yavdr(nfs)/Ghibli/Die_letzten_Glühwürmchen__1988_/2026-04-18.21.32.1-0.rec\"")
        != std::string::npos);
    assert(request.body.find(
        "\"target\":\"__vdr_suite_move_probe__~Die letzten Glühwürmchen (1988)\"")
        != std::string::npos);
    assert(request.body.find("\"copy_only\":false") != std::string::npos);

    assert(request.body.find(
        "\"source\":\"/srv/vdr/video/Ghibli/") == std::string::npos);
    assert(request.body.find(
        "__vdr_suite_move_probe__~Die_letzten_Glühwürmchen__1988_") == std::string::npos);
    assert(request.body.find("/api/") == std::string::npos);

    RecordingActionJobPayload rootPayload;
    rootPayload.backendId = "local-vdr";
    rootPayload.recordingId = "move-root-test";
    rootPayload.type = RecordingActionType::Move;
    rootPayload.dryRun = true;
    rootPayload.parameters["recordingPath"] =
        "/Archive/Move_Root_Test/2026-07-13.10.00.1-0.rec";
    rootPayload.parameters["backendNativeId"] =
        "/srv/vdr/video/Archive/Move_Root_Test/2026-07-13.10.00.1-0.rec";
    rootPayload.parameters["recordingTitle"] =
        "Archive/Move Root Test";
    rootPayload.parameters["targetPath"] = "/";

    const HttpRequest rootRequest =
        builder.buildMoveRequest(config, rootPayload);

    assert(rootRequest.body.find(
        "\"target\":\"Move Root Test\"")
        != std::string::npos);
    assert(rootRequest.body.find(
        "\"target\":\"~Move Root Test\"")
        == std::string::npos);
    assert(rootRequest.body.find(
        "\"source\":\"/srv/vdr/video/Archive/Move_Root_Test/2026-07-13.10.00.1-0.rec\"")
        != std::string::npos);

    return 0;
}
