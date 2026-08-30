#pragma once

#include "DashboardController.h"

#include <memory>
#include <mutex>
#include <string>

class ContinueWatchingRepository;
class ContinueWatchingService;
class Database;
class RecentlyWatchedRepository;
class RecentlyWatchedService;
class VdrRecordingCacheRepository;

class ContinueWatchingApiRuntime
{
public:
    static ContinueWatchingApiRuntime& instance();

    bool configure(Database& database, VdrRecordingCacheRepository& recordings);
    void reset();

    bool tryHandlePost(
        const std::string& requestTarget,
        const std::string& body,
        const std::string& actorRef,
        ApiResponse& response) const;

private:
    ContinueWatchingApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<ContinueWatchingRepository> repository_;
    std::unique_ptr<ContinueWatchingService> service_;
    std::unique_ptr<RecentlyWatchedRepository> recentlyWatchedRepository_;
    std::unique_ptr<RecentlyWatchedService> recentlyWatchedService_;
    VdrRecordingCacheRepository* recordings_ = nullptr;
};
