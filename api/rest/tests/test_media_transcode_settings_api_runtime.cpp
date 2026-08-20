#include "Database.h"
#include "MediaTranscodeSettingsApiRuntime.h"

#include <cassert>
#include <cstdlib>
#include <string>

int main()
{
    unsetenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
    Database database;
    assert(database.open(":memory:"));

    auto& runtime = MediaTranscodeSettingsApiRuntime::instance();
    runtime.reset();
    assert(runtime.configure(database));
    assert(runtime.configured());

    ApiResponse response;
    assert(runtime.tryHandleGet(
        "/api/backends/backend-a/settings/media-transcode", response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"backendId\":\"backend-a\"") != std::string::npos);
    assert(response.body.find("\"managed\":false") != std::string::npos);
    assert(response.body.find("/dev/dri/") == std::string::npos);

    assert(runtime.tryHandlePost(
        "/api/backends/backend-a/settings/media-transcode",
        "{\"backendId\":\"backend-a\",\"videoEncoderMode\":\"software\",\"operationId\":\"op-1\"}",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"managedMode\":\"software\"") != std::string::npos);
    assert(runtime.resolvePolicy("backend-a").videoEncoderMode() ==
        MediaVideoEncoderMode::Software);

    assert(runtime.tryHandlePost(
        "/api/backends/backend-a/settings/media-transcode",
        "{\"backendId\":\"backend-b\",\"videoEncoderMode\":\"auto\"}",
        response));
    assert(response.statusCode == 400);
    assert(response.body.find("backend_id_mismatch") != std::string::npos);

    assert(runtime.tryHandlePost(
        "/api/backends/backend-a/settings/media-transcode",
        "{\"backendId\":\"backend-a\",\"videoEncoderMode\":\"qsv\"}",
        response));
    assert(response.statusCode == 400);

    assert(runtime.tryHandlePost(
        "/api/backends/backend-a/settings/media-transcode",
        "{\"backendId\":\"backend-a\",\"videoEncoderMode\":\"auto\",\"ffmpegArguments\":\"-c:v h264_nvenc\"}",
        response));
    assert(response.statusCode == 400);
    assert(response.body.find("invalid_settings_payload") != std::string::npos);

    assert(runtime.tryHandlePost(
        "/api/backends/backend-a/settings/media-transcode",
        "{\"backendId\":\"backend-a\",\"clearManagedOverride\":true}",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"managed\":false") != std::string::npos);

    runtime.reset();
    assert(!runtime.configured());
    return 0;
}
