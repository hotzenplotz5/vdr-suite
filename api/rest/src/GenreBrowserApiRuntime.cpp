#include "GenreBrowserApiRuntime.h"

#include "GenreBrowserController.h"
#include "GenreIndexRepository.h"
#include "IEpgScraperMetadataResolver.h"
#include "RestQueryParameters.h"
#include "VdrEvent.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr std::int64_t DefaultEpgWindowSeconds = 48 * 60 * 60;
constexpr std::int64_t ResolverFreshnessSeconds = 6 * 60 * 60;

std::string requestPath(const std::string& requestTarget)
{
    const std::size_t query = requestTarget.find('?');
    return requestTarget.substr(0, query);
}

std::string requestQuery(const std::string& requestTarget)
{
    const std::size_t query = requestTarget.find('?');
    return query == std::string::npos
        ? std::string()
        : requestTarget.substr(query + 1);
}

std::int64_t parseInt64(
    const std::string& value,
    std::int64_t fallback)
{
    if (value.empty()) return fallback;
    try
    {
        std::size_t parsed = 0;
        const long long result = std::stoll(value, &parsed);
        if (parsed != value.size()) return fallback;
        return static_cast<std::int64_t>(result);
    }
    catch (...)
    {
        return fallback;
    }
}

std::int64_t nowEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

ApiResponse unavailableResponse()
{
    ApiResponse response;
    response.statusCode = 503;
    response.contentType = "application/json";
    response.body = "{\"error\":\"genre browser runtime is not configured\"}";
    return response;
}

std::string backendFrom(const RestQueryParameters& query)
{
    const std::string backend = query.get("backend");
    if (!backend.empty()) return backend;
    return query.get("backendId", "default");
}

std::string genreFrom(const RestQueryParameters& query)
{
    const std::string genre = query.get("genre");
    return genre.empty() ? query.get("genreId") : genre;
}
}

GenreBrowserApiRuntime& GenreBrowserApiRuntime::instance()
{
    static GenreBrowserApiRuntime runtime;
    return runtime;
}

bool GenreBrowserApiRuntime::configure(
    Database& database,
    BackendRegistryService& backendRegistryService)
{
    auto repository = std::make_unique<GenreIndexRepository>(database);
    if (!repository->ensureSchema()) return false;
    auto controller = std::make_unique<GenreBrowserController>(
        *repository,
        backendRegistryService);

    std::lock_guard<std::mutex> lock(mutex_);
    repository_ = std::move(repository);
    controller_ = std::move(controller);
    epgResolvers_.clear();
    return true;
}

void GenreBrowserApiRuntime::registerEpgScraperMetadataResolver(
    const std::string& backendId,
    IEpgScraperMetadataResolver& resolver)
{
    std::lock_guard<std::mutex> lock(mutex_);
    epgResolvers_[GenreIndexRepository::normalizeBackendId(backendId)] = &resolver;
}

bool GenreBrowserApiRuntime::refreshRecordingIndex(
    const std::string& backendId)
{
    GenreIndexRepository* repository = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        repository = repository_.get();
    }
    return repository != nullptr &&
        repository->synchronizeRecordingCache(backendId);
}

bool GenreBrowserApiRuntime::refreshEpgIndex(
    const std::string& backendId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int enrichmentLimit)
{
    GenreIndexRepository* repository = nullptr;
    IEpgScraperMetadataResolver* resolver = nullptr;
    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        repository = repository_.get();
        const auto resolverEntry = epgResolvers_.find(normalizedBackendId);
        if (resolverEntry != epgResolvers_.end()) resolver = resolverEntry->second;
    }
    if (repository == nullptr) return false;

    const std::int64_t now = nowEpochSeconds();
    if (fromTime <= 0) fromTime = now;
    if (untilTime <= fromTime) untilTime = fromTime + DefaultEpgWindowSeconds;
    if (!repository->synchronizeEpgCache(
            normalizedBackendId,
            fromTime,
            untilTime))
    {
        return false;
    }
    if (resolver == nullptr) return true;

    const std::vector<GenreEpgRefreshCandidate> candidates =
        repository->epgRefreshCandidates(
            normalizedBackendId,
            fromTime,
            untilTime,
            "tvscraper",
            now - ResolverFreshnessSeconds,
            enrichmentLimit);

    bool success = true;
    for (const GenreEpgRefreshCandidate& candidate : candidates)
    {
        VdrEvent event;
        event.id = candidate.eventId;
        event.channelId = candidate.channelId;
        event.title = candidate.title;
        event.subtitle = candidate.subtitle;
        event.description = candidate.description;
        event.startTime = candidate.startTime;
        event.endTime = candidate.endTime;
        event.durationSeconds = candidate.durationSeconds;
        event.contentDescriptors = candidate.contentDescriptors;

        EpgScraperMetadataResolution resolution;
        try
        {
            resolution = resolver->resolve(normalizedBackendId, event);
        }
        catch (...)
        {
            resolution.attempted = true;
            resolution.found = false;
        }

        if (!resolution.attempted) continue;

        GenreEvidenceInput evidence;
        evidence.backendId = normalizedBackendId;
        evidence.targetType = "program-event";
        evidence.resourceKey = candidate.channelId + "\n" + candidate.eventId;
        evidence.nativeId = candidate.eventId;
        evidence.channelId = candidate.channelId;
        evidence.startTime = parseInt64(candidate.startTime, 0);
        evidence.endTime = parseInt64(candidate.endTime, 0);
        evidence.providerId = "tvscraper";
        evidence.sourceKind = "scraper-metadata";
        evidence.observedAt = now;

        if (resolution.found && resolution.metadata.valid())
        {
            evidence.providerId = resolution.metadata.provider;
            evidence.originalValues = resolution.metadata.genres;
            evidence.state = evidence.originalValues.empty()
                ? "missing"
                : "active";
            evidence.confidence = evidence.originalValues.empty() ? 0.0 : 0.95;
        }
        else
        {
            evidence.state = "stale";
            evidence.confidence = 0.0;
        }

        if (!repository->replaceEvidence(evidence)) success = false;
    }
    return success;
}

bool GenreBrowserApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);
    if (path != "/api/metadata/genres" &&
        path != "/api/metadata/genres/recordings" &&
        path != "/api/metadata/genres/epg")
    {
        return false;
    }

    GenreBrowserController* controller = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        controller = controller_.get();
    }
    if (controller == nullptr)
    {
        response = unavailableResponse();
        return true;
    }

    const RestQueryParameters query =
        RestQueryParameters::parse(requestQuery(requestTarget));
    const std::string backendId = backendFrom(query);
    const std::int64_t fromTime = parseInt64(query.get("from"), -1);
    const std::int64_t untilTime = parseInt64(query.get("until"), -1);

    if (path == "/api/metadata/genres")
    {
        response = controller->getOverview(
            backendId,
            query.get("scope", "recordings"),
            query.get("locale", "de"),
            fromTime,
            untilTime);
        return true;
    }

    if (path == "/api/metadata/genres/recordings")
    {
        response = controller->getRecordings(
            backendId,
            genreFrom(query),
            query.getInt("limit", 48),
            query.getInt("offset", 0));
        return true;
    }

    response = controller->getEpg(
        backendId,
        genreFrom(query),
        fromTime,
        untilTime,
        query.getInt("limit", 48),
        query.getInt("offset", 0));
    return true;
}

bool GenreBrowserApiRuntime::configured() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return repository_ != nullptr && controller_ != nullptr;
}

void GenreBrowserApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    epgResolvers_.clear();
    controller_.reset();
    repository_.reset();
}
