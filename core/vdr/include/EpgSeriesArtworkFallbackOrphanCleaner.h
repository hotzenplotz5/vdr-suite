#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class EpgSeriesArtworkFallbackRepository;

struct EpgSeriesArtworkFallbackOrphanCleanupConfig
{
    bool enabled = false;
    std::string cacheRoot =
        "/var/cache/vdr-suite/epg-artwork/external";
    std::int64_t minimumAgeSeconds = 7 * 24 * 60 * 60;
    std::size_t maximumFilesPerRun = 64U;

    bool valid() const
    {
        return !cacheRoot.empty() &&
            minimumAgeSeconds >= 60 * 60 &&
            maximumFilesPerRun > 0U &&
            maximumFilesPerRun <= 1024U;
    }
};

struct EpgSeriesArtworkFallbackOrphanCleanupResult
{
    bool attempted = false;
    bool rootAvailable = false;
    bool limitReached = false;
    std::size_t examinedFiles = 0U;
    std::size_t referencedFiles = 0U;
    std::size_t youngFiles = 0U;
    std::size_t removedFiles = 0U;
    std::size_t skippedUnsafeEntries = 0U;
    std::size_t errors = 0U;

    bool succeeded() const
    {
        return !attempted || errors == 0U;
    }
};

class EpgSeriesArtworkFallbackOrphanCleaner
{
public:
    EpgSeriesArtworkFallbackOrphanCleaner(
        EpgSeriesArtworkFallbackRepository& repository,
        EpgSeriesArtworkFallbackOrphanCleanupConfig config = {});

    EpgSeriesArtworkFallbackOrphanCleanupResult cleanup(
        std::int64_t nowEpochSeconds);

private:
    EpgSeriesArtworkFallbackRepository& repository_;
    EpgSeriesArtworkFallbackOrphanCleanupConfig config_;
};
