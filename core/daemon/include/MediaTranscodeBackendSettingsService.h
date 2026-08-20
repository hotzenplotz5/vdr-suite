#pragma once

#include "Database.h"
#include "MediaTranscodePolicy.h"

#include <functional>
#include <mutex>
#include <optional>
#include <string>

struct MediaTranscodeBackendSettingsSnapshot
{
    std::string backendId;
    bool managed = false;
    std::string managedMode;
    std::string effectiveMode = "auto";
    std::string configurationSource = "default";
    MediaTranscodePolicyDiagnostics diagnostics;
};

struct MediaTranscodeBackendSettingsUpdate
{
    std::string backendId;
    std::string videoEncoderMode;
    bool clearManagedOverride = false;
};

struct MediaTranscodeBackendSettingsUpdateResult
{
    bool success = false;
    int statusCode = 500;
    std::string errorCode;
    std::string message;
    MediaTranscodeBackendSettingsSnapshot settings;
};

class MediaTranscodeBackendSettingsService
{
public:
    using VaapiHostCapabilityProbe = std::function<bool()>;

    MediaTranscodeBackendSettingsService(
        Database& database,
        std::string backendId,
        VaapiHostCapabilityProbe vaapiHostCapabilityProbe = {});

    bool ensureSchema();

    MediaTranscodeBackendSettingsSnapshot get() const;

    MediaTranscodeBackendSettingsUpdateResult update(
        const MediaTranscodeBackendSettingsUpdate& request);

    bool resolvePolicy(MediaTranscodePolicy& policy) const;

    const std::string& backendId() const
    {
        return backendId_;
    }

    static bool validBackendId(const std::string& backendId);
    static bool validManagedMode(const std::string& mode);

private:
    bool ensureSchemaLocked() const;
    bool readManagedModeLocked(std::string& mode) const;
    bool storeManagedModeLocked(const std::string& mode) const;
    bool clearManagedModeLocked() const;
    bool vaapiHostCapabilityLocked() const;
    static bool defaultVaapiEncoderCapability();
    MediaTranscodeBackendSettingsSnapshot snapshotLocked() const;
    MediaTranscodePolicy resolvePolicyLocked(
        const std::optional<MediaVideoEncoderMode>& managedMode) const;

    Database& database_;
    std::string backendId_;
    VaapiHostCapabilityProbe vaapiHostCapabilityProbe_;
    mutable std::optional<bool> cachedVaapiHostCapability_;
    mutable std::mutex mutex_;
};
