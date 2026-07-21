#include "EpgArtworkEnrichmentService.h"

#include "IEpgScraperMetadataResolverProvider.h"

#include <chrono>

namespace
{
long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

EpgArtworkEnrichmentService::EpgArtworkEnrichmentService(
    EpgArtworkRepository& repository,
    IEpgArtworkResolver& resolver,
    std::size_t maximumQueuedEvents,
    std::chrono::seconds resolvedArtworkTtl,
    std::chrono::milliseconds notFoundTtl,
    std::chrono::milliseconds retryBackoff)
    : repository_(repository),
      resolver_(resolver),
      maximumQueuedEvents_(maximumQueuedEvents),
      resolvedArtworkTtl_(resolvedArtworkTtl),
      notFoundTtl_(notFoundTtl),
      retryBackoff_(retryBackoff),
      worker_(&EpgArtworkEnrichmentService::workerLoop, this)
{
    auto* provider = dynamic_cast<IEpgScraperMetadataResolverProvider*>(
        &resolver_);
    IEpgScraperMetadataResolver* metadataResolver =
        provider == nullptr ? nullptr : provider->scraperMetadataResolver();

    if (metadataResolver != nullptr)
    {
        auto personRepository = std::make_unique<EpgPersonIndexRepository>(
            repository_.database());
        if (personRepository->ensureSchema())
        {
            personEnrichmentService_ =
                std::make_unique<EpgPersonEnrichmentService>(
                    *personRepository,
                    *metadataResolver,
                    maximumQueuedEvents,
                    notFoundTtl,
                    retryBackoff);
            personIndexRepository_ = std::move(personRepository);
        }
    }
}

EpgArtworkEnrichmentService::~EpgArtworkEnrichmentService()
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

EpgArtworkEnrichmentResult EpgArtworkEnrichmentService::enrich(
    const std::string& backendId,
    const std::vector<VdrEvent>& events)
{
    if (personEnrichmentService_ != nullptr)
    {
        personEnrichmentService_->enrich(backendId, events);
    }

    EpgArtworkEnrichmentResult result;
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

bool EpgArtworkEnrichmentService::waitUntilIdle(
    std::chrono::milliseconds timeout)
{
    const Clock::time_point deadline = Clock::now() + timeout;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!idleChanged_.wait_until(lock, deadline, [this]() {
                return queue_.empty() && !workerBusy_;
            }))
        {
            return false;
        }
    }

    if (personEnrichmentService_ == nullptr)
    {
        return true;
    }

    const Clock::time_point now = Clock::now();
    if (now >= deadline)
    {
        return false;
    }

    return personEnrichmentService_->waitUntilIdle(
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now));
}

void EpgArtworkEnrichmentService::workerLoop()
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

void EpgArtworkEnrichmentService::process(const WorkItem& item)
{
    const EpgArtworkReference cached = repository_.find(
        item.backendId,
        item.event.channelId,
        item.event.id);

    if (cached.valid() &&
        cached.resolvedAt > 0 &&
        cached.resolvedAt >= epochSeconds() - resolvedArtworkTtl_.count())
    {
        return;
    }

    const EpgArtworkResolution resolution = resolver_.resolve(
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

    EpgArtworkReference artwork = resolution.artwork;
    artwork.backendId = item.backendId;
    artwork.channelId = item.event.channelId;
    artwork.eventId = item.event.id;

    if (!repository_.upsert(artwork))
    {
        suppressUntil(item.key, retryBackoff_);
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    suppressedUntilByKey_.erase(item.key);
}

void EpgArtworkEnrichmentService::suppressUntil(
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

bool EpgArtworkEnrichmentService::isSuppressedLocked(
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

std::string EpgArtworkEnrichmentService::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string EpgArtworkEnrichmentService::makeKey(
    const std::string& backendId,
    const VdrEvent& event)
{
    return backendId + '\x1f' + event.channelId + '\x1f' + event.id;
}
