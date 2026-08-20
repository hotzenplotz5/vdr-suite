#pragma once

#include "DashboardController.h"
#include "MediaTranscodePolicy.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

class Database;
class MediaTranscodeBackendSettingsService;

class MediaTranscodeSettingsApiRuntime
{
public:
    static MediaTranscodeSettingsApiRuntime& instance();

    bool configure(Database& database);
    bool configured() const;
    void reset();

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        ApiResponse& response) const;

    bool resolvePolicy(
        const std::string& backendId,
        MediaTranscodePolicy& policy) const;

    MediaTranscodePolicy resolvePolicy(
        const std::string& backendId) const;

private:
    MediaTranscodeSettingsApiRuntime() = default;

    MediaTranscodeBackendSettingsService* findOrCreateService(
        const std::string& backendId) const;

    mutable std::mutex mutex_;
    Database* database_ = nullptr;
    mutable std::map<
        std::string,
        std::unique_ptr<MediaTranscodeBackendSettingsService>> services_;
};
