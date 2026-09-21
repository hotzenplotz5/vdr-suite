#include "GenreBrowserApiRuntime.h"

#include "GenreIndexRepository.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace
{
std::int64_t snapshotEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string snapshotMediaType(EpgScraperMediaType mediaType)
{
    switch (mediaType)
    {
    case EpgScraperMediaType::Series:
        return "series";
    case EpgScraperMediaType::Movie:
        return "movie";
    case EpgScraperMediaType::None:
        break;
    }
    return {};
}
}

bool GenreBrowserApiRuntime::applyEpgTypeSnapshot(
    const std::string& backendId,
    const std::vector<SuiteBridgeEpgTypeSnapshotTransportItem>& items)
{
    GenreIndexRepository* repository = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        repository = writerRepository_.get();
    }
    if (repository == nullptr)
    {
        return false;
    }

    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    const std::int64_t observedAt = snapshotEpochSeconds();
    bool success = true;

    for (const SuiteBridgeEpgTypeSnapshotTransportItem& item : items)
    {
        const std::string mediaType = snapshotMediaType(item.mediaType);
        if (item.channelId.empty() || item.eventId.empty() ||
            item.startTime <= 0 || item.endTime <= item.startTime ||
            mediaType.empty())
        {
            success = false;
            continue;
        }

        GenreEvidenceInput evidence;
        evidence.backendId = normalizedBackendId;
        evidence.targetType = "program-event";
        evidence.resourceKey = item.channelId + "\n" + item.eventId;
        evidence.nativeId = item.eventId;
        evidence.channelId = item.channelId;
        evidence.startTime = item.startTime;
        evidence.endTime = item.endTime;
        evidence.providerId = "tvscraper-media-type";
        evidence.sourceKind = "scraper-media-type";
        evidence.originalValues = {mediaType};
        evidence.state = "active";
        evidence.confidence = 0.99;
        evidence.observedAt = observedAt;

        const bool stored = repository->replaceEvidence(evidence);
        const bool reconciled = stored &&
            repository->reconcileEpgBrowseClassification(
                normalizedBackendId,
                evidence.resourceKey);
        if (!stored || !reconciled)
        {
            success = false;
        }
    }

    return success;
}
