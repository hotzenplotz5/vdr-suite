#include "RestfulApiRecordingActionRequestBuilder.h"

#include <cassert>
#include <string>

int main()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "default";
    config.basePath = "/api";

    RestfulApiRecordingActionRequestBuilder builder;

    RecordingActionJobPayload movePayload;
    movePayload.backendId = "default";
    movePayload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    movePayload.type = RecordingActionType::Move;
    movePayload.parameters["recordingPath"] =
        "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    movePayload.parameters["targetPath"] =
        "Archive/Crime";

    const HttpRequest moveRequest =
        builder.buildMoveRequest(config, movePayload);

    assert(moveRequest.method == "POST");
    assert(moveRequest.url == "/api/recordings/move.json");
    assert(moveRequest.headers.at("Accept") == "application/json");
    assert(moveRequest.headers.at("Content-Type") == "application/json");
    assert(moveRequest.body.find(
        "\"source\":\"Movies/Tatort/2026-06-16.20.15.1-0.rec\"")
        != std::string::npos);
    assert(moveRequest.body.find("\"target\":\"Archive~Crime~Tatort\"")
        != std::string::npos);
    assert(moveRequest.body.find("\"copy_only\":false")
        != std::string::npos);

    RecordingActionJobPayload renamePayload;
    renamePayload.backendId = "default";
    renamePayload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    renamePayload.type = RecordingActionType::Rename;
    renamePayload.parameters["recordingPath"] =
        "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    renamePayload.parameters["newName"] =
        "Movies/Tatort_Renamed";

    const HttpRequest renameRequest =
        builder.buildRenameRequest(config, renamePayload);

    assert(renameRequest.method == "POST");
    assert(renameRequest.url == "/api/recordings/move.json");
    assert(renameRequest.body.find(
        "\"source\":\"Movies/Tatort/2026-06-16.20.15.1-0.rec\"")
        != std::string::npos);
    assert(renameRequest.body.find("\"target\":\"Movies~Tatort_Renamed\"")
        != std::string::npos);
    assert(renameRequest.body.find("\"copy_only\":false")
        != std::string::npos);

    RecordingActionJobPayload deletePayload;
    deletePayload.backendId = "default";
    deletePayload.recordingId = "Movies/Tatort/2026-06-16.20.15.1-0.rec";
    deletePayload.type = RecordingActionType::Delete;
    deletePayload.parameters["recordingPath"] =
        "Movies/Tatort/2026-06-16.20.15.1-0.rec";

    const HttpRequest deleteRequest =
        builder.buildDeleteRequest(config, deletePayload);

    assert(deleteRequest.method == "POST");
    assert(deleteRequest.url == "/api/recordings/delete.json");
    assert(deleteRequest.headers.at("Accept") == "application/json");
    assert(deleteRequest.headers.at("Content-Type") == "application/json");
    assert(deleteRequest.body.find(
        "\"file\":\"Movies/Tatort/2026-06-16.20.15.1-0.rec\"")
        != std::string::npos);

    RestfulApiRecordingActionBackendConfig emptyBasePathConfig;
    emptyBasePathConfig.backendId = "default";
    emptyBasePathConfig.basePath = "";

    const HttpRequest noBasePathDeleteRequest =
        builder.buildDeleteRequest(emptyBasePathConfig, deletePayload);

    assert(noBasePathDeleteRequest.url == "/recordings/delete.json");

    return 0;
}
