#include "GenreBrowserApiRuntime.h"

#include "Database.h"
#include "EpgMetadataMaterializationQueue.h"
#include "GenreBrowserController.h"
#include "GenreIndexRepository.h"
#include "IEpgScraperMetadataResolver.h"
#include "RestQueryParameters.h"
#include "VdrEvent.h"

#include <sqlite3.h>

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
constexpr std::int64_t TransportRetrySeconds = 60;
constexpr int MaximumEnrichmentBatchSize = 64;

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

void normalizeEpgWindow(
    std::int64_t& fromTime,
    std::int64_t& untilTime)
{
    const std::int64_t now = nowEpochSeconds();
    if (fromTime <= 0) fromTime = now;
    if (untilTime <= fromTime) untilTime = fromTime + DefaultEpgWindowSeconds;
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

std::string contentClassFrom(const RestQueryParameters& query)
{
    const std::string contentClass = query.get("contentClass");
    return contentClass.empty() ? query.get("class") : contentClass;
}

std::string mediaTypeValue(EpgScraperMediaType mediaType)
{
    switch (mediaType)
    {
    case EpgScraperMediaType::Movie:
        return "movie";
    case EpgScraperMediaType::Series:
        return "series";
    case EpgScraperMediaType::None:
        break;
    }
    return {};
}

bool enrichEpgIndex(
    GenreIndexRepository& repository,
    IEpgScraperMetadataResolver* resolver,
    const std::string& normalizedBackendId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int enrichmentLimit)
{
    if (resolver == nullptr || enrichmentLimit <= 0) return true;

    bool success = true;
    int remaining = enrichmentLimit;

    while (remaining > 0)
    {
        const int batchLimit = std::min(
            remaining,
            MaximumEnrichmentBatchSize);
        const std::int64_t now = nowEpochSeconds();
        const std::vector<GenreEpgRefreshCandidate> candidates =
            repository.epgRefreshCandidates(
                normalizedBackendId,
                fromTime,
                untilTime,
                "tvscraper-media-type",
                now - ResolverFreshnessSeconds,
                batchLimit);

        if (candidates.empty()) break;

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
                resolution = EpgScraperMetadataResolution{};
            }

            GenreEvidenceInput genreEvidence;
            genreEvidence.backendId = normalizedBackendId;
            genreEvidence.targetType = "program-event";
            genreEvidence.resourceKey =
                candidate.channelId + "\n" + candidate.eventId;
            genreEvidence.nativeId = candidate.eventId;
            genreEvidence.channelId = candidate.channelId;
            genreEvidence.startTime = parseInt64(candidate.startTime, 0);
            genreEvidence.endTime = parseInt64(candidate.endTime, 0);
            genreEvidence.providerId = "tvscraper";
            genreEvidence.sourceKind = "scraper-metadata";
            genreEvidence.observedAt = resolution.attempted
                ? now
                : now - ResolverFreshnessSeconds + TransportRetrySeconds;

            GenreEvidenceInput mediaTypeEvidence = genreEvidence;
            mediaTypeEvidence.providerId = "tvscraper-media-type";
            mediaTypeEvidence.sourceKind = "scraper-media-type";

            if (resolution.found && resolution.metadata.valid())
            {
                genreEvidence.providerId = resolution.metadata.provider;
                genreEvidence.originalValues = resolution.metadata.genres;
                genreEvidence.state = genreEvidence.originalValues.empty()
                    ? "missing"
                    : "active";
                genreEvidence.confidence =
                    genreEvidence.originalValues.empty() ? 0.0 : 0.95;

                const std::string mediaType =
                    mediaTypeValue(resolution.metadata.mediaType);
                if (!mediaType.empty())
                {
                    mediaTypeEvidence.originalValues = {mediaType};
                    mediaTypeEvidence.state = "active";
                    mediaTypeEvidence.confidence = 0.99;
                }
                else
                {
                    mediaTypeEvidence.state = "stale";
                    mediaTypeEvidence.confidence = 0.0;
                }
            }
            else
            {
                genreEvidence.state = "stale";
                genreEvidence.confidence = 0.0;
                mediaTypeEvidence.state = "stale";
                mediaTypeEvidence.confidence = 0.0;
            }

            const bool genresStored =
                repository.replaceEvidence(genreEvidence);
            const bool mediaTypeStored =
                repository.replaceEvidence(mediaTypeEvidence);
            const bool browseReconciled =
                genresStored &&
                mediaTypeStored &&
                repository.reconcileEpgBrowseClassification(
                    normalizedBackendId,
                    genreEvidence.resourceKey);
            if (!genresStored || !mediaTypeStored || !browseReconciled)
            {
                success = false;
            }
        }

        remaining -= static_cast<int>(candidates.size());
        if (static_cast<int>(candidates.size()) < batchLimit) break;
    }

    return success;
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
    database.execute("PRAGMA journal_mode=WAL;");
    database.execute("PRAGMA synchronous=NORMAL;");
    database.execute("PRAGMA busy_timeout=5000;");

    auto writerRepository = std::make_unique<GenreIndexRepository>(database);
    if (!writerRepository->ensureSchema()) return false;

    std::unique_ptr<Database> readDatabase;
    std::unique_ptr<GenreIndexRepository> readRepository;
    GenreIndexRepository* controllerRepository = writerRepository.get();

    const char* databaseFilename = sqlite3_db_filename(database.handle(), "main");
    if (databaseFilename != nullptr && databaseFilename[0] != '\0')
    {
        auto candidateDatabase = std::make_unique<Database>();
        if (candidateDatabase->open(databaseFilename))
        {
            candidateDatabase->execute("PRAGMA busy_timeout=5000;");
            auto candidateRepository =
                std::make_unique<GenreIndexRepository>(*candidateDatabase);
            if (candidateRepository->ensureSchema())
            {
                candidateDatabase->execute("PRAGMA query_only=ON;");
                controllerRepository = candidateRepository.get();
                readRepository = std::move(candidateRepository);
                readDatabase = std::move(candidateDatabase);
            }
        }
    }

    auto controller = std::make_unique<GenreBrowserController>(
        *controllerRepository,
        backendRegistryService);

    std::lock_guard<std::mutex> lock(mutex_);
    controller_.reset();
    readRepository_.reset();
    readDatabase_.reset();
    writerRepository_.reset();

    writerRepository_ = std::move(writerRepository);
    readDatabase_ = std::move(readDatabase);
    readRepository_ = std::move(readRepository);
    controller_ = std::move(controller);
    epgResolvers_.clear();
    EpgMetadataMaterializationQueue::instance().reset();
    return true;
}

void GenreBrowserApiRuntime::registerEpgScraperMetadataResolver(
    const std::string& backendId,
    IEpgScraperMetadataResolver& resolver)
{
    std::lock_guard<std::mutex> lock(mutex_);
    epgResolvers_[GenreIndexRepository::normalizeBackendId(backendId)] = &resolver;
}

int GenreBrowserApiRuntime::processRequestedEpgMetadata(int maximumRequests)
{
    const std::vector<EpgMetadataMaterializationRequest> requests =
        EpgMetadataMaterializationQueue::instance().take(maximumRequests);
    int processed = 0;

    for (const EpgMetadataMaterializationRequest& request : requests)
    {
        IEpgScraperMetadataResolver* resolver = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto resolverEntry = epgResolvers_.find(request.backendId);
            if (resolverEntry != epgResolvers_.end())
            {
                resolver = resolverEntry->second;
            }
        }

        EpgScraperMetadataResolution resolution;
        if (resolver != nullptr)
        {
            VdrEvent event;
            event.channelId = request.channelId;
            event.id = request.eventId;

            try
            {
                resolution = resolver->resolve(request.backendId, event);
            }
            catch (...)
            {
                resolution = EpgScraperMetadataResolution{};
            }
        }

        EpgMetadataMaterializationOutcome outcome =
            EpgMetadataMaterializationOutcome::TransportFailure;
        if (resolution.attempted)
        {
            outcome = resolution.found && resolution.metadata.valid()
                ? EpgMetadataMaterializationOutcome::Success
                : EpgMetadataMaterializationOutcome::NotFound;
        }

        EpgMetadataMaterializationQueue::instance().complete(
            request.key,
            outcome);
        ++processed;
    }

    return processed;
}

bool GenreBrowserApiRuntime::refreshRecordingIndex(
    const std::string& backendId)
{
    GenreIndexRepository* repository = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        repository = writerRepository_.get();
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
        repository = writerRepository_.get();
        const auto resolverEntry = epgResolvers_.find(normalizedBackendId);
        if (resolverEntry != epgResolvers_.end()) resolver = resolverEntry->second;
    }
    if (repository == nullptr) return false;

    normalizeEpgWindow(fromTime, untilTime);
    if (!repository->synchronizeEpgCache(
            normalizedBackendId,
            fromTime,
            untilTime))
    {
        return false;
    }

    return enrichEpgIndex(
        *repository,
        resolver,
        normalizedBackendId,
        fromTime,
        untilTime,
        enrichmentLimit);
}

bool GenreBrowserApiRuntime::continueEpgEnrichment(
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
        repository = writerRepository_.get();
        const auto resolverEntry = epgResolvers_.find(normalizedBackendId);
        if (resolverEntry != epgResolvers_.end()) resolver = resolverEntry->second;
    }
    if (repository == nullptr) return false;

    normalizeEpgWindow(fromTime, untilTime);
    return enrichEpgIndex(
        *repository,
        resolver,
        normalizedBackendId,
        fromTime,
        untilTime,
        enrichmentLimit);
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
        contentClassFrom(query),
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
    return writerRepository_ != nullptr && controller_ != nullptr;
}

void GenreBrowserApiRuntime::reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    EpgMetadataMaterializationQueue::instance().reset();
    epgResolvers_.clear();
    controller_.reset();
    readRepository_.reset();
    readDatabase_.reset();
    writerRepository_.reset();
}
