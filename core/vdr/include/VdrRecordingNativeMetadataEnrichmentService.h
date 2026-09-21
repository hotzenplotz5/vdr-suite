#pragma once

#include "IVdrRecordingNativeMetadataResolver.h"
#include "VdrRecording.h"
#include "VdrRecordingNativeMetadataRepository.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

struct VdrRecordingNativeMetadataEnrichmentConfig
{
    std::int64_t foundTtlSeconds = 7 * 24 * 60 * 60;
    std::int64_t negativeTtlSeconds = 6 * 60 * 60;
    std::int64_t retryInitialSeconds = 30;
    std::int64_t retryMaximumSeconds = 30 * 60;
    int maximumRetryCount = 5;
    std::size_t maximumQueuedRecordings = 2048;
    int maximumBatchSize = 4;
};

struct VdrRecordingNativeMetadataEnrichmentStatus
{
    std::size_t queuedCount = 0;
    std::uint64_t resolvedFound = 0;
    std::uint64_t resolvedNotFound = 0;
    std::uint64_t failedAttempts = 0;
    std::uint64_t exhaustedRecordings = 0;
    std::uint64_t invalidRecordings = 0;
    std::string lastError;
};

class VdrRecordingNativeMetadataEnrichmentService
{
public:
    VdrRecordingNativeMetadataEnrichmentService(
        std::string backendId,
        VdrRecordingNativeMetadataRepository& repository,
        IVdrRecordingNativeMetadataResolver& resolver,
        VdrRecordingNativeMetadataEnrichmentConfig config = {});

    std::size_t reconcileInventory(
        const std::vector<VdrRecording>& recordings,
        std::int64_t now);

    int processBatch(std::int64_t now, int requestedLimit = 0);

    void clearQueue();
    VdrRecordingNativeMetadataEnrichmentStatus status() const;

private:
    struct QueueEntry
    {
        std::string backendNativeId;
        std::string recordingKey;
    };

    std::string backendId_;
    VdrRecordingNativeMetadataRepository& repository_;
    IVdrRecordingNativeMetadataResolver& resolver_;
    VdrRecordingNativeMetadataEnrichmentConfig config_;

    mutable std::mutex mutex_;
    std::deque<QueueEntry> queue_;
    std::unordered_set<std::string> queuedKeys_;
    VdrRecordingNativeMetadataEnrichmentStatus status_;

    bool shouldQueue(
        const VdrRecordingNativeMetadataRecord& record,
        std::int64_t now) const;
    bool enqueueLocked(const QueueEntry& entry);
    bool pop(QueueEntry& entry);
    bool persistResult(
        const QueueEntry& entry,
        const VdrRecordingNativeMetadata& metadata,
        std::int64_t now);
    std::int64_t retryDelaySeconds(int retryCount) const;
};
