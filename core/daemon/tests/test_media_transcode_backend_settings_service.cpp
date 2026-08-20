#include "Database.h"
#include "MediaTranscodeBackendSettingsService.h"

#include <cassert>
#include <cstdlib>

namespace
{
void setEncoderEnvironment(const char* value)
{
    if (value == nullptr) unsetenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
    else setenv("VDR_SUITE_MEDIA_VIDEO_ENCODER", value, 1);
}
}

int main()
{
    unsetenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE");
    setEncoderEnvironment(nullptr);

    Database database;
    assert(database.open(":memory:"));
    MediaTranscodeBackendSettingsService settings(database, "backend-a");
    assert(settings.ensureSchema());

    auto snapshot = settings.get();
    assert(snapshot.backendId == "backend-a");
    assert(!snapshot.managed);
    assert(snapshot.configurationSource == "default");
    assert(snapshot.effectiveMode == "auto");

    setEncoderEnvironment("software");
    snapshot = settings.get();
    assert(!snapshot.managed);
    assert(snapshot.configurationSource == "environment");
    assert(snapshot.effectiveMode == "software");

    MediaTranscodeBackendSettingsUpdate update;
    update.backendId = "backend-a";
    update.videoEncoderMode = "vaapi";
    auto result = settings.update(update);
    assert(result.success);
    assert(result.settings.managed);
    assert(result.settings.managedMode == "vaapi");
    assert(result.settings.configurationSource == "managed");
    assert(result.settings.effectiveMode == "vaapi");

    MediaTranscodePolicy resolved;
    assert(settings.resolvePolicy(resolved));
    assert(resolved.videoEncoderMode() == MediaVideoEncoderMode::Vaapi);

    MediaTranscodeBackendSettingsService reopened(database, "backend-a");
    snapshot = reopened.get();
    assert(snapshot.managed);
    assert(snapshot.managedMode == "vaapi");

    update.videoEncoderMode = "nvenc";
    result = settings.update(update);
    assert(!result.success);
    assert(result.statusCode == 400);
    assert(result.errorCode == "invalid_video_encoder_mode");

    update.videoEncoderMode.clear();
    update.clearManagedOverride = true;
    result = settings.update(update);
    assert(result.success);
    assert(!result.settings.managed);
    assert(result.settings.configurationSource == "environment");
    assert(result.settings.effectiveMode == "software");

    database.close();
    snapshot = settings.get();
    assert(snapshot.backendId.empty());
    MediaTranscodePolicy unavailable;
    assert(!settings.resolvePolicy(unavailable));

    setEncoderEnvironment(nullptr);
    return 0;
}
