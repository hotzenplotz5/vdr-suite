#pragma once

#include "DashboardController.h"

#include <map>
#include <mutex>
#include <string>

class SeriesArtworkBackendSettingsService;

class SeriesArtworkSettingsApiRuntime
{
public:
    static SeriesArtworkSettingsApiRuntime& instance();

    void registerBackend(
        const std::string& backendId,
        SeriesArtworkBackendSettingsService& service);

    void reset();

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        ApiResponse& response) const;

private:
    SeriesArtworkSettingsApiRuntime() = default;

    SeriesArtworkBackendSettingsService* findService(
        const std::string& backendId) const;

    mutable std::mutex mutex_;
    std::map<std::string, SeriesArtworkBackendSettingsService*> services_;
};
