#include "VdrRecordingNativeMetadataEnrichmentService.h"

#include "VdrRecordingNativeIdentity.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <utility>

namespace
{
std::string normalizedBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

bool retryableFailure(VdrRecordingNativeMetadataAvailability availability)
{
    return availability == VdrRecordingNativeMetadataAvailability::ProviderUnavailable ||
        availability == VdrRecordingNativeMetadataAvailability::TransportError ||
        availability == VdrRecordingNativeMetadataAvailability::InvalidPayload;
}
}

VdrRecordingNativeMetadataEnrichmentService::VdrRecordingNativeMetadataEnrichmentService(
    std::string backendId,
    VdrRecordingNativeMetadataRepository& repository,
    IVdrRecordingNativeMetadataResolver& resolver,
    VdrRecordingNativeMetadataEnrichmentConfig config)
    : backendId_(normalizedBackendId(backendId)),
      repository_(repository),
      resolver_(resolver),
      config_(config)
{
    config_.foundTtlSeconds = std::max<std::int64_t>(1, config_.foundTtlSeconds);
    config_.negativeTtlSeconds = std::max<std::int64_t>(1, config_.negativeTtlSeconds);
    config_.retryInitialSeconds = std::max<std::int64_t>(1, config_.retryInitialSeconds);
    config_.retryMaximumSeconds = std::max(config_.retryInitialSeconds, config_.retryMaximumSeconds);
    config_.maximumRetryCount = std::max(1, config_.maximumRetryCount);
    config_.maximumQueuedRecordings = std::max<std::size_t>(1, config_.maximumQueuedRecordings);
    config_.maximumBatchSize = std::max(1, config_.maximumBatchSize);
}

std::size_t VdrRecordingNativeMetadataEnrichmentService::reconcileInventory(
    const std::vector<VdrRecording>& recordings,
    std::int64_t now)
{
    std::unordered_map<std::string, std::string> nativeIdByKey;
    std::unordered_set<std::string> ambiguousKeys;
    std::vector<std::string> activeKeys;
    activeKeys.reserve(recordings.size());

    for (const VdrRecording& recording : recordings)
    {
        const std::string key = VdrRecordingNativeIdentity::keyForNativeId(recording.backendNativeId);
        if (key.empty())
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++status_.invalidRecordings;
            status_.lastError = "recording has no valid backend-native identity";
            continue;
        }

        const auto inserted = nativeIdByKey.emplace(key, recording.backendNativeId);
        if (!inserted.second && inserted.first->second != recording.backendNativeId)
        {
            ambiguousKeys.insert(key);
        }
    }

    for (const auto& entry : nativeIdByKey)
    {
        if (ambiguousKeys.find(entry.first) == ambiguousKeys.end())
        {
            activeKeys.push_back(entry.first);
        }
    }

    if (!repository_.removeMissingRecordings(backendId_, activeKeys))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        status_.lastError = "failed to remove metadata for missing recordings";
    }

    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::string& key : ambiguousKeys)
    {
        ++status_.invalidRecordings;
        status_.lastError = "ambiguous recording identity key";
        queuedKeys_.erase(key);
    }

    for (const auto& entry : nativeIdByKey)
    {
        if (ambiguousKeys.find(entry.first) != ambiguousKeys.end())
        {
            continue;
        }

        const VdrRecordingNativeMetadataRecord record = repository_.find(backendId_, entry.first);
        if (!shouldQueue(record, now))
        {
            continue;
        }

        if (record.exists() && record.retryCount >= config_.maximumRetryCount)
        {
            ++status_.exhaustedRecordings;
            continue;
        }

        enqueueLocked({entry.second, entry.first});
    }

    status_.queuedCount = queue_.size();
    return queue_.size();
}

bool VdrRecordingNativeMetadataEnrichmentService::shouldQueue(
    const VdrRecordingNativeMetadataRecord& record,
    std::int64_t now) const
{
    if (!record.exists())
    {
        return true;
    }

    if (record.retryCount > 0)
    {
        return record.retryCount < config_.maximumRetryCount &&
            record.nextRetryAt > 0 && record.nextRetryAt <= now;
    }

    if (record.contentState == "found")
    {
        return record.expiresAt <= now;
    }
    if (record.contentState == "not_found")
    {
        return record.negativeExpiresAt <= now;
    }
    return true;
}

bool VdrRecordingNativeMetadataEnrichmentService::enqueueLocked(
    const QueueEntry& entry)
{
    if (queue_.size() >= config_.maximumQueuedRecordings ||
        !queuedKeys_.insert(entry.recordingKey).second)
    {
        return false;
    }
    queue_.push_back(entry);
    return true;
}

bool VdrRecordingNativeMetadataEnrichmentService::pop(QueueEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
    {
        return false;
    }
    entry = std::move(queue_.front());
    queue_.pop_front();
    queuedKeys_.erase(entry.recordingKey);
    status_.queuedCount = queue_.size();
    return true;
}

int VdrRecordingNativeMetadataEnrichmentService::processBatch(
    std::int64_t now,
    int requestedLimit)
{
    const int limit = requestedLimit > 0
        ? std::min(requestedLimit, config_.maximumBatchSize)
        : config_.maximumBatchSize;

    int processed = 0;
    QueueEntry entry;
    while (processed < limit && pop(entry))
    {
        const VdrRecordingNativeMetadata metadata = resolver_.resolve(entry.recordingKey);
        persistResult(entry, metadata, now);
        ++processed;
    }
    return processed;
}

bool VdrRecordingNativeMetadataEnrichmentService::persistResult(
    const QueueEntry& entry,
    const VdrRecordingNativeMetadata& metadata,
    std::int64_t now)
{
    if (metadata.availability == VdrRecordingNativeMetadataAvailability::Found ||
        metadata.availability == VdrRecordingNativeMetadataAvailability::NotFound)
    {
        const bool found = metadata.availability == VdrRecordingNativeMetadataAvailability::Found;
        const bool stored = repository_.storeResolution(
            backendId_,
            entry.backendNativeId,
            metadata,
            now,
            found ? now + config_.foundTtlSeconds : 0,
            found ? 0 : now + config_.negativeTtlSeconds);

        std::lock_guard<std::mutex> lock(mutex_);
        if (stored)
        {
            if (found) ++status_.resolvedFound;
            else ++status_.resolvedNotFound;
            status_.lastError.clear();
        }
        else
        {
            ++status_.failedAttempts;
            status_.lastError = "failed to persist recording metadata resolution";
        }
        return stored;
    }

    if (!retryableFailure(metadata.availability))
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++status_.failedAttempts;
        status_.lastError = "unsupported recording metadata resolution state";
        return false;
    }

    const VdrRecordingNativeMetadataRecord previous = repository_.find(backendId_, entry.recordingKey);
    const int retryCount = std::min(previous.retryCount + 1, config_.maximumRetryCount);
    const std::int64_t nextRetryAt = retryCount >= config_.maximumRetryCount
        ? 0
        : now + retryDelaySeconds(retryCount);
    const std::string diagnostic = metadata.diagnostic.empty()
        ? "recording metadata resolution failed"
        : metadata.diagnostic;
    const bool stored = repository_.recordFailure(
        backendId_,
        entry.backendNativeId,
        entry.recordingKey,
        metadata.availability,
        diagnostic,
        retryCount,
        nextRetryAt);

    std::lock_guard<std::mutex> lock(mutex_);
    ++status_.failedAttempts;
    if (retryCount >= config_.maximumRetryCount)
    {
        ++status_.exhaustedRecordings;
    }
    status_.lastError = diagnostic;
    return stored;
}

std::int64_t VdrRecordingNativeMetadataEnrichmentService::retryDelaySeconds(
    int retryCount) const
{
    std::int64_t delay = config_.retryInitialSeconds;
    for (int attempt = 1; attempt < retryCount; ++attempt)
    {
        if (delay >= config_.retryMaximumSeconds ||
            delay > std::numeric_limits<std::int64_t>::max() / 2)
        {
            return config_.retryMaximumSeconds;
        }
        delay = std::min(delay * 2, config_.retryMaximumSeconds);
    }
    return delay;
}

void VdrRecordingNativeMetadataEnrichmentService::clearQueue()
{
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
    queuedKeys_.clear();
    status_.queuedCount = 0;
}

VdrRecordingNativeMetadataEnrichmentStatus
VdrRecordingNativeMetadataEnrichmentService::status() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    VdrRecordingNativeMetadataEnrichmentStatus result = status_;
    result.queuedCount = queue_.size();
    return result;
}
