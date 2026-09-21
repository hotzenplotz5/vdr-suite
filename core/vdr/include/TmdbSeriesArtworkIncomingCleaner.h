#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct TmdbSeriesArtworkIncomingCleanupConfig
{
    bool enabled = false;
    std::string incomingRoot;
    std::int64_t minimumAgeSeconds = 24 * 60 * 60;
    std::size_t maximumFilesPerRun = 64U;

    bool valid() const;
};

struct TmdbSeriesArtworkIncomingCleanupResult
{
    bool attempted = false;
    bool rootAvailable = false;
    std::size_t examinedEntries = 0U;
    std::size_t recognizedFiles = 0U;
    std::size_t youngFiles = 0U;
    std::size_t removedCandidateFiles = 0U;
    std::size_t removedTemporaryFiles = 0U;
    std::size_t skippedForeignEntries = 0U;
    std::size_t skippedUnsafeEntries = 0U;
    std::size_t errors = 0U;
    bool limitReached = false;

    std::size_t removedFiles() const
    {
        return removedCandidateFiles + removedTemporaryFiles;
    }

    bool succeeded() const
    {
        return errors == 0U;
    }
};

class TmdbSeriesArtworkIncomingCleaner
{
public:
    explicit TmdbSeriesArtworkIncomingCleaner(
        TmdbSeriesArtworkIncomingCleanupConfig config);

    TmdbSeriesArtworkIncomingCleanupResult cleanup(
        std::int64_t nowEpochSeconds) const;

private:
    TmdbSeriesArtworkIncomingCleanupConfig config_;
};
