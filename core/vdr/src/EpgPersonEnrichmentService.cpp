#include "EpgPersonEnrichmentService.h"

EpgPersonEnrichmentService::EpgPersonEnrichmentService(
    EpgPersonIndexRepository& repository,
    IEpgScraperMetadataResolver& resolver,
    std::size_t maximumQueuedEvents,
    std::chrono::milliseconds notFoundTtl,
    std::chrono::milliseconds retryBackoff)
    : repository_(repository),
      resolver_(resolver),
      maximumQueuedEvents_(maximumQueuedEvents),
      notFoundTtl_(notFoundTtl),
      retryBackoff_(retryBackoff),
      worker_(&EpgPersonEnrichmentService::workerLoop, this)
{
}

EpgPersonEnrichmentService::~EpgPersonEnrichmentService()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopRequested_ = true;
        queue_.clear();
        pendingKeys_.clear();
    }

    workAvailable_.notify_all();
    idleChanged_.notify_all();

    if (worker_.joinable())
    {
        worker_.join();
    }
}

EpgPersonEnrichmentResult EpgPersonEnrichmentService::enrich(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    EpgPersonEnrichmentResult result;
    const std::string normalizedBackendId = normalizeBackendId(backendId);

    std::lock_guard<std::mutex> lock(mutex_);
    if (stopRequested_)
    {
        result.queueAvailable = false;
        result.dropped = events.size();
        return result;
    }

    const Clock::time_point now = Clock::now();

    for (const VdrEvent& event : events)
    {
        if (event.id.empty() || event.channelId.empty())
        {
            ++result.dropped;
            continue;
        }

        const std::string key = makeKey(normalizedBackendId, event);
        if (pendingKeys_.find(key) != pendingKeys_.end())
        {
            ++result.deduplicated;
            continue;
        }

        if (isSuppressedLocked(key, now))
        {
            ++result.suppressed;
            continue;
        }

        if (pendingKeys_.size() >= maximumQueuedEvents_)
        {
            ++result.dropped;
            continue;
        }

        WorkItem item;
        item.backendId = normalizedBackendId;
        item.event = event;
        item.key = key;
        pendingKeys_.insert(key);
        queue_.push_back(std::move(item));
        ++result.queued;
    }

    if (result.queued > 0)
    {
        workAvailable_.notify_one();
    }

    return result;
}

bool EpgPersonEnrichmentService::waitUntilIdle(
    std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex_);
    return idleChanged_.wait_for(lock, timeout, [this]() {
        return queue_.empty() && !workerBusy_;
    });
}

void EpgPersonEnrichmentService::workerLoop()
{
    while (true)
    {
        WorkItem item;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            workAvailable_.wait(lock, [this]() {
                return stopRequested_ || !queue_.empty();
            });

            if (stopRequested_)
            {
                workerBusy_ = false;
                idleChanged_.notify_all();
                return;
            }

            item = std::move(queue_.front());
            queue_.pop_front();
            workerBusy_ = true;
        }

        try
        {
            process(item);
        }
        catch (...)
        {
            suppressUntil(item.key, retryBackoff_);
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            pendingKeys_.erase(item.key);
            workerBusy_ = false;
            if (queue_.empty())
            {
                idleChanged_.notify_all();
            }
        }
    }
}

void EpgPersonEnrichmentService::process(const WorkItem& item)
{
    const EpgScraperMetadataResolution resolution = resolver_.resolve(
        item.backendId,
        item.event);

    if (!resolution.attempted)
    {
        suppressUntil(item.key, retryBackoff_);
        return;
    }

    if (!resolution.found)
    {
        if (!repository_.removeForEvent(
                item.backendId,
                item.event.channelId,
                item.event.id))
        {
            suppressUntil(item.key, retryBackoff_);
            return;
        }

        suppressUntil(item.key, notFoundTtl_);
        return;
    }

    if (!repository_.replaceEvidenceForEvent(
            item.backendId,
            item.event,
            resolution.metadata))
    {
        suppressUntil(item.key, retryBackoff_);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    suppressedUntilByKey_.erase(item.key);
}

void EpgPersonEnrichmentService::suppressUntil(
    const std::string& key,
    std::chrono::milliseconds duration)
{
    if (duration.count() <= 0)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    suppressedUntilByKey_[key] = Clock::now() + duration;
}

bool EpgPersonEnrichmentService::isSuppressedLocked(
    const std::string& key,
    Clock::time_point now)
{
    const auto found = suppressedUntilByKey_.find(key);
    if (found == suppressedUntilByKey_.end())
    {
        return false;
    }

    if (found->second <= now)
    {
        suppressedUntilByKey_.erase(found);
        return false;
    }

    return true;
}

std::string EpgPersonEnrichmentService::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string EpgPersonEnrichmentService::makeKey(
    const std::string& backendId,
    const VdrEvent& event)
{
    return backendId + '\x1f' + event.channelId + '\x1f' + event.id;
}
