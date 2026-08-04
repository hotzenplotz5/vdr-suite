#pragma once

#include "DashboardController.h"
#include "MetadataController.h"

#include <map>
#include <mutex>
#include <string>

class ManualRecordingMetadataApiRuntime
{
public:
    static ManualRecordingMetadataApiRuntime& instance();

    void registerController(MetadataController& controller);
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
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response) const;

private:
    ManualRecordingMetadataApiRuntime() = default;

    MetadataController* controller() const;

    mutable std::mutex mutex_;
    MetadataController* controller_ = nullptr;
};
