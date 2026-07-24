#include "GenreBrowserApiRuntime.h"

#include "Database.h"
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
constexpr std::size_t MaximumPendingMetadataRequests = 128;
constexpr auto MetadataNotFoundBackoff = std::chrono::minutes(5);
constexpr auto MetadataTransportBackoff = std::chrono::seconds(30);

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

std::string metadataRequestKey(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    return backendId + '\x1f' + channelId + '\x1f' + eventId;
}

bool enrichEpgIndex(
    GenreIndexRepository& repository,
    IEpgScraperMetadataResolver* resolver,
    const std::string& normalizedBackendId,
    std::int64_t fromTime,
    std::int64_t untilTime,
    int enrichmentLimit)
{
    if (resolver == nullptr) return true;

    const std::int64_t now = nowEpochSeconds();
    const std::vector<GenreEpgRefreshCandidate> candidates =
        repository.epgRefreshCandidates(
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

        if (!repository.replaceEvidence(evidence)) success = false;
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
    epgMetadataRequests_.clear();
    epgMetadataRequestKeys_.clear();
    epgMetadataSuppressedUntil_.clear();
    return true;
}

void GenreBrowserApiRuntime::registerEpgScraperMetadataResolver(
    const std::string& backendId,
    IEpgScraperMetadataResolver& resolver)
{
    std::lock_guard<std::mutex> lock(mutex_);
    epgResolvers_[GenreIndexRepository::normalizeBackendId(backendId)] = &resolver;
}

void GenreBrowserApiRuntime::requestEpgMetadataMaterialization(
    const std::string& backendId,
    const std::string& channelId,
    const std::string& eventId)
{
    if (channelId.empty() || eventId.empty()) return;

    const std::string normalizedBackendId =
        GenreIndexRepository::normalizeBackendId(backendId);
    const std::string key = metadataRequestKey(
        normalizedBackendId,
        channelId,
        eventId);
    const auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mutex_);

    const auto suppressed = epgMetadataSuppressedUntil_.find(key);
    if (suppressed != epgMetadataSuppressedUntil_.end())
    {
        if (suppressed->second > now) return;
        epgMetadataSuppressedUntil_.erase(suppressed);
    }

    if (epgMetadataRequestKeys_.find(key) != epgMetadataRequestKeys_.end())
    {
        return;
    }

    if (epgMetadataRequests_.size() >= MaximumPendingMetadataRequests)
    {
        return;
    }

    EpgMetadataMaterializationRequest request;
    request.backendId = normalizedBackendId;
    request.channelId = channelId;
    request.eventId = eventId;
    request.key = key;
    epgMetadataRequestKeys_.insert(key);
    epgMetadataRequests_.push_back(std::move(request));
}

int GenreBrowserApiRuntime::processRequestedEpgMetadata(int maximumRequests)
{
    const int boundedMaximum = std::max(1, std::min(maximumRequests, 16));
    int processed = 0;

    while (processed < boundedMaximum)
    {
        EpgMetadataMaterializationRequest request;
        IEpgScraperMetadataResolver* resolver = nullptr;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (epgMetadataRequests_.empty()) break;

            request = std::move(epgMetadataRequests_.front());
            epgMetadataRequests_.pop_front();

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

        {
            std::lock_guard<std::mutex> lock(mutex_);
            epgMetadataRequestKeys_.erase(request.key);

            if (!resolution.attempted)
            {
                epgMetadataSuppressedUntil_[request.key] =
                    std::chrono::steady_clock::now() + MetadataTransportBackoff;
            }
            else if (!resolution.found || !resolution.metadata.valid())
            {
                epgMetadataSuppressedUntil_[request.key] =
                    std::chrono::steady_clock::now() + MetadataNotFoundBackoff;
            }
            else
            {
                epgMetadataSuppressedUntil_.erase(request.key);
            }
        }

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
    epgResolvers_.clear();
    epgMetadataRequests_.clear();
    epgMetadataRequestKeys_.clear();
    epgMetadataSuppressedUntil_.clear();
    controller_.reset();
    readRepository_.reset();
    readDatabase_.reset();
    writerRepository_.reset();
}
