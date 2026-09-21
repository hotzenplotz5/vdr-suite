#include "MediaTranscodeBackendSettingsService.h"

#include "MediaProcessRunner.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <utility>

namespace
{
constexpr const char* FfmpegExecutable = "/usr/bin/ffmpeg";
constexpr auto VaapiCapabilityProbeTimeout = std::chrono::seconds(3);
constexpr std::size_t MaximumVaapiCapabilityProbeOutputBytes = 64U * 1024U;

std::optional<MediaVideoEncoderMode> parsedMode(const std::string& value)
{
    bool valid = false;
    const MediaVideoEncoderMode mode =
        MediaTranscodePolicy::videoEncoderModeFromString(value, valid);
    return valid ? std::optional<MediaVideoEncoderMode>(mode) : std::nullopt;
}
}

MediaTranscodeBackendSettingsService::MediaTranscodeBackendSettingsService(
    Database& database,
    std::string backendId,
    VaapiHostCapabilityProbe vaapiHostCapabilityProbe)
    : repository_(database),
      backendId_(std::move(backendId)),
      vaapiHostCapabilityProbe_(std::move(vaapiHostCapabilityProbe))
{
}

bool MediaTranscodeBackendSettingsService::validBackendId(
    const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9') ||
                    character == '-' || character == '_' || character == '.';
            });
}

bool MediaTranscodeBackendSettingsService::validManagedMode(
    const std::string& mode)
{
    return mode == "auto" || mode == "software" || mode == "vaapi";
}

bool MediaTranscodeBackendSettingsService::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return validBackendId(backendId_) && ensureSchemaLocked();
}

bool MediaTranscodeBackendSettingsService::ensureSchemaLocked() const
{
    return repository_.ensureSchema();
}

bool MediaTranscodeBackendSettingsService::readManagedModeLocked(
    std::string& mode) const
{
    return repository_.readManagedMode(backendId_, mode);
}

bool MediaTranscodeBackendSettingsService::storeManagedModeLocked(
    const std::string& mode) const
{
    return validManagedMode(mode) &&
        repository_.storeManagedMode(backendId_, mode);
}

bool MediaTranscodeBackendSettingsService::clearManagedModeLocked() const
{
    return repository_.clearManagedMode(backendId_);
}

bool MediaTranscodeBackendSettingsService::defaultVaapiEncoderCapability(
    const std::string& vaapiDevice)
{
    if (vaapiDevice.empty() ||
        vaapiDevice.size() > 512U ||
        vaapiDevice.rfind("/dev/dri/", 0) != 0 ||
        ::access(vaapiDevice.c_str(), R_OK | W_OK) != 0)
    {
        return false;
    }

    const MediaProcessCaptureResult result = MediaProcessRunner().runAndCapture(
        {
            FfmpegExecutable,
            "-hide_banner",
            "-loglevel", "error",
            "-init_hw_device", "vaapi=va:" + vaapiDevice,
            "-h", "encoder=h264_vaapi"
        },
        "/",
        VaapiCapabilityProbeTimeout,
        MaximumVaapiCapabilityProbeOutputBytes);
    return result.success;
}

bool MediaTranscodeBackendSettingsService::vaapiHostCapabilityLocked(
    const std::string& vaapiDevice) const
{
    if (cachedVaapiHostCapability_.has_value() &&
        cachedVaapiHostCapabilityDevice_ == vaapiDevice)
    {
        return cachedVaapiHostCapability_.value();
    }

    const bool available = vaapiHostCapabilityProbe_
        ? vaapiHostCapabilityProbe_(vaapiDevice)
        : defaultVaapiEncoderCapability(vaapiDevice);
    cachedVaapiHostCapabilityDevice_ = vaapiDevice;
    cachedVaapiHostCapability_ = available;
    return available;
}

MediaTranscodePolicy MediaTranscodeBackendSettingsService::resolvePolicyLocked(
    const std::optional<MediaVideoEncoderMode>& managedMode) const
{
    if (vaapiHostCapabilityProbe_) {
        MediaTranscodePolicy injected = MediaTranscodePolicy::fromEnvironment(
            managedMode,
            true);
        return MediaTranscodePolicy::fromEnvironment(
            managedMode,
            vaapiHostCapabilityLocked(injected.vaapiDevice()));
    }

    MediaTranscodePolicy policy = MediaTranscodePolicy::fromEnvironment(managedMode);
    if (!policy.diagnostics().vaapiAvailable) return policy;
    if (vaapiHostCapabilityLocked(policy.vaapiDevice())) return policy;
    return MediaTranscodePolicy::fromEnvironment(managedMode, false);
}

MediaTranscodeBackendSettingsSnapshot
MediaTranscodeBackendSettingsService::snapshotLocked() const
{
    MediaTranscodeBackendSettingsSnapshot snapshot;
    if (!validBackendId(backendId_)) return snapshot;

    std::string managedMode;
    if (!readManagedModeLocked(managedMode)) return snapshot;

    std::optional<MediaVideoEncoderMode> managed;
    if (!managedMode.empty()) {
        if (!validManagedMode(managedMode)) return snapshot;
        managed = parsedMode(managedMode);
        if (!managed.has_value()) return snapshot;
    }

    snapshot.backendId = backendId_;
    snapshot.managed = managed.has_value();
    snapshot.managedMode = snapshot.managed ? managedMode : std::string();

    if (snapshot.managed) {
        snapshot.configurationSource = "managed";
    }
    else {
        const char* raw = std::getenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
        const std::optional<MediaVideoEncoderMode> environment =
            raw == nullptr ? std::nullopt : parsedMode(raw);
        snapshot.configurationSource = environment.has_value()
            ? "environment"
            : "default";
    }

    const MediaTranscodePolicy policy = resolvePolicyLocked(managed);
    snapshot.effectiveMode =
        MediaTranscodePolicy::videoEncoderModeName(policy.videoEncoderMode());
    snapshot.diagnostics = policy.diagnostics();
    return snapshot;
}

MediaTranscodeBackendSettingsSnapshot
MediaTranscodeBackendSettingsService::get() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId_)) return {};
    return snapshotLocked();
}

MediaTranscodeBackendSettingsUpdateResult
MediaTranscodeBackendSettingsService::update(
    const MediaTranscodeBackendSettingsUpdate& request)
{
    MediaTranscodeBackendSettingsUpdateResult result;
    if (request.backendId != backendId_ || !validBackendId(request.backendId)) {
        result.statusCode = 400;
        result.errorCode = "backend_id_mismatch";
        result.message = "The backend ID in the route and payload must match";
        return result;
    }

    if (request.clearManagedOverride) {
        if (!request.videoEncoderMode.empty()) {
            result.statusCode = 400;
            result.errorCode = "conflicting_encoder_setting";
            result.message = "Deployment default cannot be combined with a managed encoder mode";
            return result;
        }
    }
    else if (!validManagedMode(request.videoEncoderMode)) {
        result.statusCode = 400;
        result.errorCode = "invalid_video_encoder_mode";
        result.message = "The video encoder mode must be auto, software, or vaapi";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const bool stored = request.clearManagedOverride
        ? clearManagedModeLocked()
        : storeManagedModeLocked(request.videoEncoderMode);
    if (!stored) {
        result.statusCode = 503;
        result.errorCode = "media_transcode_settings_persistence_failed";
        result.message = "Media transcode settings could not be persisted";
        return result;
    }

    result.settings = snapshotLocked();
    if (result.settings.backendId.empty()) {
        result.statusCode = 503;
        result.errorCode = "media_transcode_settings_readback_failed";
        result.message = "Media transcode settings could not be read back after persistence";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    return result;
}

bool MediaTranscodeBackendSettingsService::resolvePolicy(
    MediaTranscodePolicy& policy) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId_)) return false;

    std::string managedMode;
    if (!readManagedModeLocked(managedMode)) return false;

    std::optional<MediaVideoEncoderMode> managed;
    if (!managedMode.empty()) {
        if (!validManagedMode(managedMode)) return false;
        managed = parsedMode(managedMode);
        if (!managed.has_value()) return false;
    }

    policy = resolvePolicyLocked(managed);
    return true;
}
