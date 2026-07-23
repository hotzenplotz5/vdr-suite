#pragma once

#include "DashboardController.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
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
    GenreBrowserApiRuntime() = default;

    mutable std::mutex mutex_;
    std::unique_ptr<GenreIndexRepository> writerRepository_;
    std::unique_ptr<Database> readDatabase_;
    std::unique_ptr<GenreIndexRepository> readRepository_;
    std::unique_ptr<GenreBrowserController> controller_;
    std::map<std::string, IEpgScraperMetadataResolver*> epgResolvers_;
};
