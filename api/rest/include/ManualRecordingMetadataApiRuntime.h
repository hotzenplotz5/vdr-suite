#pragma once

#include "DashboardController.h"
#include "MetadataController.h"

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
