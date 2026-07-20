#pragma once

#include "EpgMetadataRepository.h"
#include "IEpgMetadataResolver.h"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct EpgMetadataEnrichmentResult
{
    std::size_t queued = 0;
    std::size_t deduplicated = 0;
    std::size_t suppressed = 0;
    std::size_t dropped = 0;
    bool queueAvailable = true;
};

class EpgMetadataEnrichmentService
{
public:
    EpgMetadataEnrichmentService(
        EpgMetadataRepository& repository,
        IEpgMetadataResolver& resolver,
        std::size_t maximumQueuedEvents = 256,
        std::chrono::seconds resolvedMetadataTtl = std::chrono::hours(24),
        std::chrono::milliseconds notFoundTtl = std::chrono::hours(6),
        std::chrono::milliseconds retryBackoff = std::chrono::minutes(5));

    ~EpgMetadataEnrichmentService();

    EpgMetadataEnrichmentService(const EpgMetadataEnrichmentService&) = delete;
    EpgMetadataEnrichmentService& operator=(
        const EpgMetadataEnrichmentService&) = delete;

    EpgMetadataEnrichmentResult enrich(
        const std::string& backendId,
        const std::vector<VdrEvent>& events);

    bool waitUntilIdle(std::chrono::milliseconds timeout);

private:
    using Clock = std::chrono::steady_clock;

    struct WorkItem
    {
        std::string backendId;
        VdrEvent event;
        std::string key;
    };

    EpgMetadataRepository& repository_;
    IEpgMetadataResolver& resolver_;
    const std::size_t maximumQueuedEvents_;
    const std::chrono::seconds resolvedMetadataTtl_;
    const std::chrono::milliseconds notFoundTtl_;
    const std::chrono::milliseconds retryBackoff_;

    std::mutex mutex_;
    std::condition_variable workAvailable_;
    std::condition_variable idleChanged_;
    std::deque<WorkItem> queue_;
    std::unordered_set<std::string> pendingKeys_;
    std::unordered_map<std::string, Clock::time_point> suppressedUntilByKey_;
    std::thread worker_;
    bool stopRequested_ = false;
    bool workerBusy_ = false;

    void workerLoop();
    void process(const WorkItem& item);
    void suppressUntil(
        const std::string& key,
        std::chrono::milliseconds duration);
    bool isSuppressedLocked(
        const std::string& key,
        Clock::time_point now);

    static std::string normalizeBackendId(const std::string& backendId);
    static std::string makeKey(
        const std::string& backendId,
        const VdrEvent& event);
};
