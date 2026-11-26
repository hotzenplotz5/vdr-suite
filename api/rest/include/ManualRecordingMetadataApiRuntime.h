#pragma once

#include "DashboardController.h"
#include "MetadataController.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

class ManualRecordingMetadataApiRuntime
{
public:
    static ManualRecordingMetadataApiRuntime& instance();

    using RecordingPresentationChangedCallback =
        std::function<void(const std::string& backendId,
                           const std::string& recordingNativeId)>;

    void registerController(MetadataController& controller);
    void registerRecordingPresentationChangedCallback(
        RecordingPresentationChangedCallback callback);
    using PublicRecordingNativeLookup = std::function<std::string(
        const std::string& backendId, const std::string& recordingId)>;
    void registerPublicRecordingNativeLookup(PublicRecordingNativeLookup callback);
    using PublicRecordingTrailerLookup = std::function<std::pair<std::string, std::string>(
        const std::string& backendId, const std::string& nativeId)>;
    void registerPublicRecordingTrailerLookup(PublicRecordingTrailerLookup callback);
    void reset();

    ManualRecordingMetadataAssignment findSelected(
        const std::string& backendId,
        const std::string& resourceKey) const
    {
        MetadataController* metadata = controller();
        return metadata == nullptr
            ? ManualRecordingMetadataAssignment{}
            : metadata->findManualRecordingMetadata(backendId, resourceKey);
    }

    std::map<std::string, ManualRecordingMetadataAssignment>
    findSelectedForBackend(
        const std::string& backendId) const
    {
        MetadataController* metadata = controller();
        return metadata == nullptr
            ? std::map<std::string, ManualRecordingMetadataAssignment>{}
            : metadata->findManualRecordingMetadataForBackend(backendId);
    }

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response,
        const std::vector<std::string>& authorizedBackendIds = {}) const;

    // Public-v1 façade: parse one canonical read-only resolve request.
    bool resolveReadOnly(
        const std::string& backendId,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response,
        const std::string& authorizedBackendId = "",
        const std::string& contentType = "") const;

private:
    ManualRecordingMetadataApiRuntime() = default;

    MetadataController* controller() const;
    void notifyRecordingPresentationChanged(
        const std::string& backendId,
        const std::string& recordingNativeId) const;

    mutable std::mutex mutex_;
    MetadataController* controller_ = nullptr;
    RecordingPresentationChangedCallback
        recordingPresentationChangedCallback_;
    PublicRecordingNativeLookup publicRecordingNativeLookup_;
    PublicRecordingTrailerLookup publicRecordingTrailerLookup_;
};
