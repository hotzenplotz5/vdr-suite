#include "RestfulApiRecordingTrashRequestBuilder.h"
#include "RestfulApiRecordingTrashResponseParser.h"

#include <cassert>

namespace
{
RestfulApiRecordingActionBackendConfig makeConfig()
{
    RestfulApiRecordingActionBackendConfig config;
    config.backendId = "default";
    config.basePath = "/api";
    return config;
}

RecordingActionJobPayload makeDeletePayload()
{
    RecordingActionJobPayload payload;
    payload.backendId = "default";
    payload.recordingId = "recording-001";
    payload.type = RecordingActionType::Delete;
    payload.parameters["recordingPath"] =
        "/Movies/Tatort/2026-07-14.20.15.1-0.rec";
    payload.parameters["backendNativeId"] =
        "/srv/vdr/video/Movies/Tatort/2026-07-14.20.15.1-0.rec";
    return payload;
}
}

int main()
{
    const auto config = makeConfig();
    const auto payload = makeDeletePayload();
    RestfulApiRecordingTrashRequestBuilder builder;

    const HttpRequest preview = builder.buildPreviewRequest(config, payload);
    assert(preview.method == "POST");
    assert(preview.url == "/api/recordings/trash/preview.json");
    assert(
        preview.body ==
        "{\"file\":\"/srv/vdr/video/Movies/Tatort/2026-07-14.20.15.1-0.rec\"}");

    const HttpRequest validate =
        builder.buildValidateRequest(config, payload, 12345, 67890);
    assert(validate.url == "/api/recordings/trash/validate.json");
    assert(
        validate.body ==
        "{\"file\":\"/srv/vdr/video/Movies/Tatort/2026-07-14.20.15.1-0.rec\","
        "\"revision_recordings_state\":\"12345\","
        "\"revision_timers_state\":\"67890\"}");

    const HttpRequest execute =
        builder.buildExecuteRequest(config, payload, 12345, 67890);
    assert(execute.url == "/api/recordings/trash.json");
    assert(execute.body == validate.body);

    const auto ready =
        RestfulApiRecordingTrashResponseParser::parsePreview(
            "{"
            "\"executable\":true,"
            "\"recording_file\":\"/srv/vdr/video/test.rec\","
            "\"blockers\":[],"
            "\"warnings\":[\"searchtimer warning\"],"
            "\"revision_recordings_state\":12345,"
            "\"revision_timers_state\":\"67890\""
            "}");
    assert(ready.parsed);
    assert(ready.executable);
    assert(ready.recordingsState == 12345);
    assert(ready.timersState == 67890);
    assert(ready.blockers.empty());
    assert(ready.warnings.size() == 1);

    const auto blocked =
        RestfulApiRecordingTrashResponseParser::parsePreview(
            "{"
            "\"executable\":false,"
            "\"recording_file\":\"/srv/vdr/video/test.rec\","
            "\"blockers\":[\"local-timer-active\",\"replay-active\"],"
            "\"warnings\":[],"
            "\"revision_recordings_state\":101,"
            "\"revision_timers_state\":202"
            "}");
    assert(blocked.parsed);
    assert(!blocked.executable);
    assert(blocked.blockers.size() == 2);

    const auto validation =
        RestfulApiRecordingTrashResponseParser::parseValidate(
            "{\"status\":\"ready\",\"blockers\":[],\"warnings\":[]}");
    assert(validation.parsed);
    assert(validation.status == "ready");

    const auto trashed =
        RestfulApiRecordingTrashResponseParser::parseExecute(
            "{"
            "\"status\":\"trashed\","
            "\"recording_file\":\"/srv/vdr/video/test.rec\","
            "\"deleted_recording_file\":\"/srv/vdr/video/test.del\","
            "\"message\":\"Recording moved to the VDR trash.\""
            "}");
    assert(trashed.parsed);
    assert(trashed.status == "trashed");
    assert(trashed.deletedRecordingFile == "/srv/vdr/video/test.del");

    const auto alreadyTrashed =
        RestfulApiRecordingTrashResponseParser::parseExecute(
            "{"
            "\"status\":\"already-trashed\","
            "\"recording_file\":\"/srv/vdr/video/test.rec\","
            "\"deleted_recording_file\":\"/srv/vdr/video/test.del\","
            "\"message\":\"Recording is already present in the VDR trash.\""
            "}");
    assert(alreadyTrashed.parsed);
    assert(alreadyTrashed.status == "already-trashed");

    const auto malformed =
        RestfulApiRecordingTrashResponseParser::parsePreview(
            "{\"executable\":true}");
    assert(!malformed.parsed);

    return 0;
}
