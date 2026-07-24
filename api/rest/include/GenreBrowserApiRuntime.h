#pragma once

#include "DashboardController.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

class BackendRegistryService;
class Database;
class GenreBrowserController;
class GenreIndexRepository;
class IEpgScraperMetadataResolver;

class GenreBrowserApiRuntime
{
public:
    static GenreBrowserApiRuntime& instance();

    bool configure(
        Database& database,
        BackendRegistryService& backendRegistryService);

    void registerEpgScraperMetadataResolver(
        const std::string& backendId,
        IEpgScraperMetadataResolver& resolver);

    void requestEpgMetadataMaterialization(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId);

    int processRequestedEpgMetadata(int maximumRequests = 4);

    bool refreshRecordingIndex(const std::string& backendId);

    bool refreshEpgIndex(
        const std::string& backendId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int enrichmentLimit = 32);

    bool continueEpgEnrichment(
        const std::string& backendId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int enrichmentLimit = 8);

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    bool configured() const;
    void reset();

private:
    struct EpgMetadataMaterializationRequest
    {
        std::string backendId;
        std::string channelId;
        std::string eventId;
        std::string key;
    };

    GenreBrowserApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<GenreIndexRepository> writerRepository_;
    std::unique_ptr<Database> readDatabase_;
    std::unique_ptr<GenreIndexRepository> readRepository_;
    std::unique_ptr<GenreBrowserController> controller_;
    std::map<std::string, IEpgScraperMetadataResolver*> epgResolvers_;
    std::deque<EpgMetadataMaterializationRequest> epgMetadataRequests_;
    std::set<std::string> epgMetadataRequestKeys_;
    std::map<std::string, std::chrono::steady_clock::time_point>
        epgMetadataSuppressedUntil_;
};
