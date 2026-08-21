#include "Database.h"
#include "MediaTranscodeBackendSettingsService.h"

#include <cassert>
#include <cstdlib>
#include <string>

namespace
{
void setEncoderEnvironment(const char* value)
{
    if (value == nullptr) unsetenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
    else setenv("VDR_SUITE_MEDIA_VIDEO_ENCODER", value, 1);
}

MediaPresentationProfile vaapiTranscodeProfile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "progressive-fmp4";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Transcode;
    profile.videoAction = MediaTrackAction::Transcode;
    profile.audioAction = MediaTrackAction::Copy;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetVideoWidth = 1280;
    profile.targetVideoHeight = 720;
    profile.videoTranscodeWorkload = MediaTranscodeWorkload::Standard;
    return profile;
}
}

int main()
{
    unsetenv("VDR_SUITE_MEDIA_TRANSCODE_PROFILE");
    unsetenv("VDR_SUITE_MEDIA_VAAPI_DEVICE");
    unsetenv("VDR_SUITE_MEDIA_X264_PRESET");
    unsetenv("VDR_SUITE_MEDIA_X264_STANDARD_PRESET");
    unsetenv("VDR_SUITE_MEDIA_X264_DEINTERLACE_PRESET");
    unsetenv("VDR_SUITE_MEDIA_X264_UHD_PRESET");
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

    int availableProbeCalls = 0;
    MediaTranscodeBackendSettingsService availableVaapi(
        database,
        "backend-vaapi-available",
        [&availableProbeCalls](const std::string& device) {
            ++availableProbeCalls;
            assert(device.rfind("/dev/dri/", 0) == 0);
            return true;
        });
    assert(availableVaapi.ensureSchema());
    MediaTranscodeBackendSettingsUpdate availableUpdate;
    availableUpdate.backendId = "backend-vaapi-available";
    availableUpdate.videoEncoderMode = "vaapi";
    auto availableResult = availableVaapi.update(availableUpdate);
    assert(availableResult.success);
    assert(availableResult.settings.diagnostics.vaapiAvailable);
    MediaTranscodePolicy availablePolicy;
    assert(availableVaapi.resolvePolicy(availablePolicy));
    const auto availableProfile = availablePolicy.apply(vaapiTranscodeProfile());
    assert(availableProfile.available);
    assert(availableProfile.videoEncoderBackend == MediaVideoEncoderBackend::Vaapi);
    assert(availableProbeCalls == 1);

    int unavailableProbeCalls = 0;
    MediaTranscodeBackendSettingsService unavailableVaapi(
        database,
        "backend-vaapi-unavailable",
        [&unavailableProbeCalls](const std::string&) {
            ++unavailableProbeCalls;
            return false;
        });
    assert(unavailableVaapi.ensureSchema());
    MediaTranscodeBackendSettingsUpdate unavailableUpdate;
    unavailableUpdate.backendId = "backend-vaapi-unavailable";
    unavailableUpdate.videoEncoderMode = "vaapi";
    auto unavailableResult = unavailableVaapi.update(unavailableUpdate);
    assert(unavailableResult.success);
    assert(!unavailableResult.settings.diagnostics.vaapiAvailable);
    MediaTranscodePolicy unavailablePolicy;
    assert(unavailableVaapi.resolvePolicy(unavailablePolicy));
    const auto unavailableProfile = unavailablePolicy.apply(vaapiTranscodeProfile());
    assert(!unavailableProfile.available);
    assert(unavailableProfile.reason ==
        "forced VAAPI is unavailable on the execution host");
    assert(unavailableProbeCalls == 1);

    database.close();
    snapshot = settings.get();
    assert(snapshot.backendId.empty());
    MediaTranscodePolicy unavailable;
    assert(!settings.resolvePolicy(unavailable));

    setEncoderEnvironment(nullptr);
    return 0;
}
