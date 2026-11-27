#include "GenreBrowserApiRuntime.h"

#include "BackendRegistryService.h"
#include "Database.h"
#include "EpgMetadataMaterializationQueue.h"
#include "GenreBrowserController.h"
#include "GenreIndexRepository.h"
#include "IEpgScraperMetadataResolver.h"
#include "PublicApiRuntime.h"
#include "PublicRecordingDisplayName.h"
#include "RestQueryParameters.h"
#include "VdrEvent.h"

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <locale>
#include <locale.h>
#include <wctype.h>
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
                "tvscraper",
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

            const bool evidenceStored =
                repository.replaceEpgEvidenceAndReconcile(
                    genreEvidence,
                    mediaTypeEvidence);
            if (!evidenceStored)
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

bool GenreBrowserApiRuntime::epgOverview(
    const std::string& backendId, const std::string& locale,
    std::int64_t fromTime, std::int64_t untilTime,
    GenreEpgBrowseOverview& result) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    GenreIndexRepository* repository = readRepository_ ?
        readRepository_.get() : writerRepository_.get();
    if (!repository || backendId.empty() || untilTime <= fromTime)
        return false;
    result = repository->epgBrowseOverview(
        backendId, fromTime, untilTime, locale);
    return true;
}
bool GenreBrowserApiRuntime::epgPage(
    const std::string& backendId, const std::string& contentClass,
    const std::string& genreId, std::int64_t fromTime,
    std::int64_t untilTime, int limit, int offset, GenreEpgPage& result) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    GenreIndexRepository* repository = readRepository_ ?
        readRepository_.get() : writerRepository_.get();
    if (!repository || backendId.empty() ||
        !GenreIndexRepository::isEpgBrowseContentClass(contentClass) ||
        (!genreId.empty() && (contentClass != "movie" ||
            !GenreIndexRepository::isFilmGenre(genreId))) ||
        untilTime <= fromTime || limit < 1 || limit > 30 ||
        offset < 0 || offset > 1000000)
        return false;
    result = repository->epgByBrowse(
        backendId, contentClass, genreId, fromTime, untilTime, limit, offset);
    return true;
}

bool GenreBrowserApiRuntime::recordingOverview(
    const std::string& backendId,
    const std::string& locale,
    GenreOverview& result) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    GenreIndexRepository* repository = readRepository_ ?
        readRepository_.get() : writerRepository_.get();
    if (!repository) return false;
    result = repository->overview(backendId, "recording", 0, 0, locale);
    return true;
}

PublicGenreCollectionResult GenreBrowserApiRuntime::recordingOverviewPage(
    const PublicGenreCollectionRequest& request,
    const BackendRegistryService& backends) const
{
    PublicGenreCollectionResult result;
    const auto backend = backends.getBackend(request.backendId);
    if (!backend.has_value() || !backend->enabled)
        return result;
    // Facets are a persisted index read, independent of live VDR snapshots
    // and native recording identity resolution.
    GenreOverview overview;
    if (!recordingOverview(
        request.backendId, request.locale, overview))
        return result;
    // Keep public ID pagination stable; expose the existing
    // canonical Web index order independently.
    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < overview.genres.size(); ++i)
        order.push_back(i);
    std::sort(order.begin(), order.end(),
        [&overview](std::size_t a, std::size_t b) {
            return overview.genres[a].genreId < overview.genres[b].genreId;
        });
    result.totalCount = overview.genres.size();
    result.totalItems = static_cast<std::size_t>(overview.distinctItemCount);
    result.assignmentCount = static_cast<std::size_t>(overview.assignmentCount);
    const std::size_t end =
        std::min(result.totalCount, request.offset + request.limit);
    for (std::size_t index = request.offset; index < end; ++index)
    {
        const auto& item = overview.genres[order[index]];
        if (item.genreId.empty() || item.label.empty() ||
            item.itemCount < 0)
            return PublicGenreCollectionResult{};
        PublicGenreItem view;
        view.genreId = item.genreId;
        view.label = item.label;
        view.labelDe = item.labelDe;
        view.labelEn = item.labelEn;
        view.count = static_cast<std::size_t>(item.itemCount);
        view.known = item.known;
        view.activeCount = static_cast<std::size_t>(item.activeCount);
        view.staleCount = static_cast<std::size_t>(item.staleCount);
        view.conflictCount = static_cast<std::size_t>(item.conflictCount);
        view.displayOrder = order[index];
        result.genres.push_back(std::move(view));
    }
    result.status = PublicGenreCollectionStatus::ok;
    return result;
}

bool GenreBrowserApiRuntime::recordingPage(
    const std::string& backendId,
    const std::string& genreId,
    int limit, int offset,
    GenreRecordingPage& result) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    GenreIndexRepository* repository = readRepository_ ?
        readRepository_.get() : writerRepository_.get();
    if (!repository || backendId.empty() || genreId.empty() ||
        limit < 1 || limit > 100 || offset < 0 || offset > 1000000)
        return false;
    result = repository->recordingsByGenre(
        backendId, genreId, limit, offset);
    return true;
}

bool GenreBrowserApiRuntime::recordingCards(
    const std::string& backendId, const std::vector<std::string>& nativeIds,
    GenreRecordingPage& result) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    GenreIndexRepository* repository = readRepository_ ?
        readRepository_.get() : writerRepository_.get();
    if (!repository || backendId.empty() || nativeIds.size() > 100U)
        return false;
    result = repository->recordingCards(backendId, nativeIds);
    return true;
}

std::string GenreBrowserApiRuntime::projectRecordingCard(
    const GenreRecordingItem& source, PublicGenreRecordingItem& target)
{
    target.sizeMb = std::max<std::int64_t>(0, source.sizeMb);
    target.genreIds = source.genreIds;
    // Match the established Genre page's locked manual/native card selection.
    // The index already selects the canonical metadata; do not resolve again.
    if (source.seriesMetadataAvailable)
    {
        target.cardTitle = source.seriesTitle;
        target.cardMediaType = "series";
        target.episodeName = source.episodeName;
        target.seasonNumber = source.seasonNumber;
        target.episodeNumber = source.episodeNumber;
    }
    else if (source.nativeCardMetadataAvailable)
    {
        target.cardTitle = source.seriesTitle;
        target.cardMediaType = source.nativeCardMediaType;
        target.episodeName = source.episodeName;
    }
    if (source.seriesHierarchyOverrideAvailable)
    {
        target.seriesHierarchyOverrideAvailable = true;
        target.seriesHierarchyGroupType = source.seriesHierarchyGroupType;
        target.seriesHierarchySeasonNumber = source.seriesHierarchySeasonNumber;
        target.seriesHierarchyGroupLabel = source.seriesHierarchyGroupLabel;
        target.seriesHierarchySortOrder = source.seriesHierarchySortOrder;
        target.seriesHierarchyEpisodeStart = source.seriesHierarchyEpisodeStart;
        target.seriesHierarchyEpisodeEnd = source.seriesHierarchyEpisodeEnd;
        target.seriesHierarchyRevision = source.seriesHierarchyRevision;
    }

    // Same initial Series membership evidence as the established Genre Web
    // payload: its provider object is empty, and embedded native metadata
    // supplies provider identity only if no folder can be used.
    if (!source.seriesMetadataAvailable &&
        std::find(source.genreIds.begin(), source.genreIds.end(), "series") ==
            source.genreIds.end())
        return {};

    target.seriesNativeMetadataAvailable = source.seriesMetadataAvailable &&
        !source.seriesProvider.empty() && source.seriesProviderId != 0;

    // The old Web grouping key used Unicode lowercase. Keep persisted
    // folder/title keys compatible independently of the daemon's locale.
    const auto lowerText = [](std::string text) {
        static const auto utf8 = newlocale(LC_CTYPE_MASK, "C.UTF-8", nullptr);
        if (utf8)
        {
            try
            {
                std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
                std::wstring lower;
                for (wchar_t c : convert.from_bytes(text))
                {
                    if (c == L'\u0130') lower += L"i\u0307";
                    else lower += static_cast<wchar_t>(towlower_l(c, utf8));
                }
                return convert.to_bytes(lower);
            }
            catch (const std::range_error&) { /* malformed legacy bytes */ }
        }
        for (char& c : text)
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
        return text;
    };
    const std::string rawPath = source.path.empty() ? source.title : source.path;
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start < rawPath.size())
    {
        const auto slash = rawPath.find('/', start);
        const auto part = PublicRecordingDisplayName::trim(rawPath.substr(
            start, slash == std::string::npos ? std::string::npos : slash - start));
        if (!part.empty()) parts.push_back(part);
        if (slash == std::string::npos) break;
        start = slash + 1U;
    }
    // The Home Series root deliberately groups every season subfolder
    // under Serien/<series>. Other paths use the immediate parent.
    std::string folder;
    for (std::size_t i = 0; i + 1U < parts.size(); ++i)
    {
        if (lowerText(parts[i]) == "serien")
        {
            folder = "Serien/" + parts[i + 1U];
            target.seriesFolderTitle = parts[i + 1U];
            break;
        }
    }
    if (folder.empty() && parts.size() >= 2U)
    {
        target.seriesFolderTitle = parts[parts.size() - 2U];
        for (std::size_t i = 0; i + 1U < parts.size(); ++i)
        {
            if (i) folder += '/';
            folder += parts[i];
        }
    }
    if (!folder.empty())
        return "folder:" + lowerText(folder);

    if (target.seriesNativeMetadataAvailable)
        return "native:" + source.seriesProvider + ":" +
            std::to_string(source.seriesProviderId);

    // Without a folder or a native Series identity, retain the existing
    // title-based fallback. Do not promote an explicit episode name.
    const auto seriesTitle = PublicRecordingDisplayName::trim(source.seriesTitle);
    const auto episodeName = PublicRecordingDisplayName::trim(source.episodeName);
    const auto title = source.seriesMetadataAvailable && !seriesTitle.empty() &&
        lowerText(seriesTitle) != lowerText(episodeName)
        ? seriesTitle : PublicRecordingDisplayName::trim(source.title);
    return "title:" + lowerText(title.empty() ? "Aufnahme" : title);
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
