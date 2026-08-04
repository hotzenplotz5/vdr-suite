#pragma once

#include "DashboardController.h"

#include <mutex>
#include <string>

class MetadataController;

class ManualRecordingMetadataApiRuntime
{
public:
    static ManualRecordingMetadataApiRuntime& instance();

    void registerController(MetadataController& controller);
    void reset();

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
