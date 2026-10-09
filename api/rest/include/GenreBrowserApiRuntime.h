#pragma once

#include "DashboardController.h"
#include "GenreIndexRepository.h"
#include "ISuiteBridgeEpgTypeSnapshotTransport.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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

    int processRequestedEpgMetadata(int maximumRequests = 4);

    bool refreshRecordingIndex(const std::string& backendId);

    bool refreshEpgIndex(
        const std::string& backendId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int enrichmentLimit = 32);

    bool applyEpgTypeSnapshot(
        const std::string& backendId,
        const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem>& items);

    bool continueEpgEnrichment(
        const std::string& backendId,
        std::int64_t fromTime,
        std::int64_t untilTime,
        int enrichmentLimit = 8);

    bool tryHandleGet(
        const std::string& requestTarget,
        ApiResponse& response) const;

    // Safe, read-only access to the same canonical Genre index used by Web.
    // Caller remains responsible for Public-v1 Device authorization and
    // public Recording identity projection.
    bool recordingOverview(
        const std::string& backendId,
        const std::string& locale,
        GenreOverview& result) const;
    bool recordingPage(
        const std::string& backendId,
        const std::string& genreId,
        int limit, int offset,
        GenreRecordingPage& result) const;

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
