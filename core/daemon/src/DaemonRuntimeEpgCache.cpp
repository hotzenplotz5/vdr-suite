#include "DaemonRuntime.h"

#include "DaemonCacheRefreshExecutionGate.h"
#include "GenreBrowserApiRuntime.h"
#include "VdrChannelCacheRepository.h"
#include "VdrEventQuery.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>
#include <unordered_map>

namespace
{
constexpr std::int64_t GenreWindowSeconds = 48 * 60 * 60;
constexpr std::size_t EpgTypeSnapshotPageSize = 64;
constexpr int InitialEpgTypeSnapshotPages = 32;
constexpr int PeriodicEpgTypeSnapshotPages = 1;
constexpr int PeriodicEpgContinuationSeconds = 1;
constexpr int PeriodicGenreEnrichmentBatchSize = 8;
constexpr int EpgTypeSnapshotRefreshSeconds = 15 * 60;

std::int64_t epgGenreEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool processEpgTypeSnapshotPages(
    BackendRuntimeContext& context,
    int maximumPages)
{
    if (!context.suiteBridgeTransport ||
        !context.epgTypeSnapshotSupported ||
        context.epgTypeSnapshotComplete ||
        context.epgTypeSnapshotUntil <= context.epgTypeSnapshotFrom ||
        maximumPages <= 0)
    {
        return true;
    }

    for (int pageIndex = 0; pageIndex < maximumPages; ++pageIndex)
    {
        const SuiteBridgeEpgTypeSnapshotTransportPage page =
            context.suiteBridgeTransport->requestEpgTypeSnapshot(
                context.epgTypeSnapshotFrom,
                context.epgTypeSnapshotUntil,
                context.epgTypeSnapshotOffset,
                EpgTypeSnapshotPageSize);

        if (!page.transportSucceeded)
        {
            if (page.replyCode == 500 ||
                page.replyCode == 501 ||
                page.replyCode == 502)
            {
                context.epgTypeSnapshotSupported = false;
                std::cout
                    << "EPG type snapshot unavailable: backend="
                    << context.backendId
                    << ", reply="
                    << page.replyCode
                    << std::endl;
            }
            else
            {
                std::cerr
                    << "EPG type snapshot transport failed: backend="
                    << context.backendId
                    << ", offset="
                    << context.epgTypeSnapshotOffset
                    << ", reply="
                    << page.replyCode
                    << std::endl;
            }
            return false;
        }

        if (!page.payloadValid ||
            (page.scanned == 0 && !page.done) ||
            page.nextOffset < context.epgTypeSnapshotOffset)
        {
            std::cerr
                << "EPG type snapshot payload invalid: backend="
                << context.backendId
                << ", offset="
                << context.epgTypeSnapshotOffset
                << std::endl;
            return false;
        }

        const bool applied = GenreBrowserApiRuntime::instance()
            .applyEpgTypeSnapshot(context.backendId, page.items);
        if (!applied)
        {
            std::cerr
                << "EPG type snapshot persistence failed: backend="
                << context.backendId
                << ", offset="
                << context.epgTypeSnapshotOffset
                << ", scanned="
                << page.scanned
                << std::endl;
            return false;
        }

        context.epgTypeSnapshotOffset = page.nextOffset;
        context.epgTypeSnapshotComplete = page.done;

        std::cout
            << "EPG type snapshot page finished: backend="
            << context.backendId
            << ", scanned="
            << page.scanned
            << ", classified="
            << page.items.size()
            << ", nextOffset="
            << page.nextOffset
            << ", done="
            << (page.done ? "true" : "false")
            << ", applied="
            << (applied ? "true" : "false")
            << std::endl;

        if (page.done)
        {
            break;
        }
    }

    return true;
}
}

void DaemonRuntime::startEpgCacheWarmupWorker()
{
    if (epgCacheWarmupThread_.joinable()) {
        return;
    }

    if (snapshotCacheService_) {
        VdrChannelCacheRepository channelCache(database_);
        if (!channelCache.ensureSchema()) {
            std::cerr
                << "failed to initialize persistent VDR channel cache"
                << std::endl;
        }
        else {
            for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                if (!backendRuntimeContext) {
                    continue;
                }

                const VdrSnapshot* snapshot =
                    snapshotCacheService_->cache().snapshotForBackend(
                        backendRuntimeContext->backendId);
                if (snapshot == nullptr) {
                    continue;
                }

                if (!channelCache.replaceChannelsForBackend(
                        backendRuntimeContext->backendId,
                        snapshot->channels)) {
                    std::cerr
                        << "failed to persist VDR channel snapshot: backend="
                        << backendRuntimeContext->backendId
                        << std::endl;
                }
                else {
                    std::cout
                        << "VDR channel snapshot persisted: backend="
                        << backendRuntimeContext->backendId
                        << ", channels="
                        << snapshot->channels.size()
                        << std::endl;
                }
            }
        }
    }

    epgCacheWarmupStopRequested_.store(false);
    epgCacheDirtyHint_.store(false);

    epgCacheWarmupThread_ = std::thread([this]() {
        runEpgCacheWarmupWorker();
    });
}

void DaemonRuntime::stopEpgCacheWarmupWorker()
{
    epgCacheWarmupStopRequested_.store(true);

    if (epgCacheWarmupThread_.joinable()) {
        epgCacheWarmupThread_.join();
    }
}

void DaemonRuntime::runEpgCacheWarmupWorker()
{
    try {
        const int initialDelaySeconds = 20;
        const int dirtyDebounceSeconds = 120;

        std::cout
            << "EPG cache warmup worker scheduled after "
            << initialDelaySeconds
            << " seconds"
            << std::endl;

        const auto waitForStop = [this](int seconds) -> bool {
            for (int elapsed = 0; elapsed < seconds; ++elapsed) {
                if (epgCacheWarmupStopRequested_.load()) {
                    return true;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            return epgCacheWarmupStopRequested_.load();
        };

        if (waitForStop(initialDelaySeconds)) {
            return;
        }

        refreshEpgCacheForAllBackends("startup");

        auto lastRefresh = std::chrono::steady_clock::now();
        auto lastEpgContinuation = lastRefresh;

        using SnapshotCompletionTime =
            std::chrono::steady_clock::time_point;
        std::unordered_map<std::string, SnapshotCompletionTime>
            epgTypeSnapshotCompletedAt;

        for (const auto& backendRuntimeContext :
             backendRuntimeContexts_)
        {
            if (backendRuntimeContext &&
                backendRuntimeContext->epgTypeSnapshotComplete)
            {
                epgTypeSnapshotCompletedAt[
                    backendRuntimeContext->backendId] =
                        lastRefresh;
            }
        }

        while (!epgCacheWarmupStopRequested_.load()) {
            if (waitForStop(1)) {
                return;
            }

            {
                auto refreshLease =
                    DaemonCacheRefreshExecutionGate::acquire();
                if (epgCacheWarmupStopRequested_.load()) {
                    return;
                }

                const int processed = GenreBrowserApiRuntime::instance()
                    .processRequestedEpgMetadata(4);
                if (processed > 0) {
                    std::cout
                        << "EPG metadata demand materialization finished: requests="
                        << processed
                        << std::endl;
                }
            }

            const auto now = std::chrono::steady_clock::now();
            const auto secondsSinceEpgContinuation =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastEpgContinuation).count();

            if (secondsSinceEpgContinuation >= PeriodicEpgContinuationSeconds) {
                auto refreshLease =
                    DaemonCacheRefreshExecutionGate::acquire();
                if (epgCacheWarmupStopRequested_.load()) {
                    return;
                }

                const std::int64_t fromTime = epgGenreEpochSeconds();
                for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                    if (epgCacheWarmupStopRequested_.load()) {
                        return;
                    }
                    if (!backendRuntimeContext) {
                        continue;
                    }

                    if (backendRuntimeContext->suiteBridgeTransport &&
                        backendRuntimeContext->epgTypeSnapshotSupported)
                    {
                        const bool snapshotStateInvalid =
                            backendRuntimeContext->epgTypeSnapshotFrom <= 0 ||
                            backendRuntimeContext->epgTypeSnapshotUntil <=
                                backendRuntimeContext->epgTypeSnapshotFrom;

                        const auto completedAt =
                            epgTypeSnapshotCompletedAt.find(
                                backendRuntimeContext->backendId);

                        const bool completedSnapshotRefreshDue =
                            backendRuntimeContext->epgTypeSnapshotComplete &&
                            (completedAt ==
                                 epgTypeSnapshotCompletedAt.end() ||
                             std::chrono::duration_cast<
                                 std::chrono::seconds>(
                                     now - completedAt->second).count() >=
                                 EpgTypeSnapshotRefreshSeconds);

                        const bool initializePeriodicSnapshot =
                            snapshotStateInvalid ||
                            completedSnapshotRefreshDue;

                        if (initializePeriodicSnapshot)
                        {
                            backendRuntimeContext->epgTypeSnapshotFrom =
                                fromTime;
                            backendRuntimeContext->epgTypeSnapshotUntil =
                                fromTime + GenreWindowSeconds;
                            backendRuntimeContext->epgTypeSnapshotOffset = 0;
                            backendRuntimeContext->epgTypeSnapshotComplete =
                                false;

                            epgTypeSnapshotCompletedAt.erase(
                                backendRuntimeContext->backendId);
                        }

                        const bool snapshotWasComplete =
                            backendRuntimeContext
                                ->epgTypeSnapshotComplete;

                        processEpgTypeSnapshotPages(
                            *backendRuntimeContext,
                            PeriodicEpgTypeSnapshotPages);

                        if (!snapshotWasComplete &&
                            backendRuntimeContext
                                ->epgTypeSnapshotComplete)
                        {
                            epgTypeSnapshotCompletedAt[
                                backendRuntimeContext->backendId] =
                                    std::chrono::steady_clock::now();
                        }
                    }

                    GenreBrowserApiRuntime::instance().continueEpgEnrichment(
                        backendRuntimeContext->backendId,
                        fromTime,
                        fromTime + GenreWindowSeconds,
                        PeriodicGenreEnrichmentBatchSize);
                }
                lastEpgContinuation = std::chrono::steady_clock::now();
            }

            if (!epgCacheDirtyHint_.load()) {
                continue;
            }

            const auto secondsSinceLastRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastRefresh).count();

            if (secondsSinceLastRefresh < dirtyDebounceSeconds) {
                continue;
            }

            if (!epgCacheDirtyHint_.exchange(false)) {
                continue;
            }

            refreshEpgCacheForAllBackends("event-stream-dirty-hint");

            const auto refreshCompletedAt =
                std::chrono::steady_clock::now();

            for (const auto& backendRuntimeContext :
                 backendRuntimeContexts_)
            {
                if (!backendRuntimeContext)
                {
                    continue;
                }

                if (backendRuntimeContext->epgTypeSnapshotComplete)
                {
                    epgTypeSnapshotCompletedAt[
                        backendRuntimeContext->backendId] =
                            refreshCompletedAt;
                }
                else
                {
                    epgTypeSnapshotCompletedAt.erase(
                        backendRuntimeContext->backendId);
                }
            }

            lastRefresh = refreshCompletedAt;
            lastEpgContinuation = lastRefresh;
        }
    }
    catch (const std::exception& error) {
        std::cerr
            << "EPG cache warmup worker stopped after exception: "
            << error.what()
            << std::endl;
    }
    catch (...) {
        std::cerr
            << "EPG cache warmup worker stopped after unknown exception"
            << std::endl;
    }
}

void DaemonRuntime::refreshEpgCacheForAllBackends(const std::string& reason)
{
    auto refreshLease = DaemonCacheRefreshExecutionGate::acquire();
    if (epgCacheWarmupStopRequested_.load()) {
        return;
    }

    if (backendRuntimeContexts_.empty()) {
        std::cout
            << "EPG cache warmup skipped: no VDR backend configured"
            << std::endl;
        return;
    }

    constexpr std::int64_t PastOverlapSeconds = 3 * 60 * 60;
    const std::int64_t currentEpoch = epgGenreEpochSeconds();

    VdrEventQuery query;
    query.from = currentEpoch - PastOverlapSeconds;
    query.timespan = static_cast<int>(
        GenreWindowSeconds + PastOverlapSeconds);
    query.channelEventLimit = 160;

    VdrChannelCacheRepository channelCache(database_);
    const bool channelCacheReady = channelCache.ensureSchema();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (epgCacheWarmupStopRequested_.load()) {
            return;
        }

        if (!backendRuntimeContext || !backendRuntimeContext->epgCacheService) {
            continue;
        }

        bool channelsStored = false;
        std::size_t channelCount = 0;
        if (channelCacheReady && snapshotCacheService_) {
            const VdrSnapshot* snapshot =
                snapshotCacheService_->cache().snapshotForBackend(
                    backendRuntimeContext->backendId);
            if (snapshot != nullptr) {
                channelCount = snapshot->channels.size();
                channelsStored = channelCache.replaceChannelsForBackend(
                    backendRuntimeContext->backendId,
                    snapshot->channels);
            }
        }

        std::cout
            << "EPG cache warmup started: reason="
            << reason
            << ", backend="
            << backendRuntimeContext->backendId
            << ", timespan="
            << query.timespan
            << ", channelEventLimit="
            << query.channelEventLimit
            << ", channelsStored="
            << (channelsStored ? "true" : "false")
            << ", channels="
            << channelCount
            << std::endl;

        const EpgCacheRefreshResult result =
            backendRuntimeContext->epgCacheService->refreshBackendWindow(
                backendRuntimeContext->backendId,
                query);

        bool genreIndexed = false;
        if (result.stored && !epgCacheWarmupStopRequested_.load()) {
            const std::int64_t fromTime = epgGenreEpochSeconds();
            genreIndexed = GenreBrowserApiRuntime::instance().refreshEpgIndex(
                backendRuntimeContext->backendId,
                fromTime,
                fromTime + GenreWindowSeconds,
                1024);

            if (backendRuntimeContext->suiteBridgeTransport &&
                backendRuntimeContext->epgTypeSnapshotSupported)
            {
                const bool initializeSnapshot =
                    backendRuntimeContext->epgTypeSnapshotComplete ||
                    backendRuntimeContext->epgTypeSnapshotFrom <= 0 ||
                    backendRuntimeContext->epgTypeSnapshotUntil <=
                        backendRuntimeContext->epgTypeSnapshotFrom;
                if (initializeSnapshot)
                {
                    backendRuntimeContext->epgTypeSnapshotFrom = fromTime;
                    backendRuntimeContext->epgTypeSnapshotUntil =
                        fromTime + GenreWindowSeconds;
                    backendRuntimeContext->epgTypeSnapshotOffset = 0;
                    backendRuntimeContext->epgTypeSnapshotComplete = false;
                }

                processEpgTypeSnapshotPages(
                    *backendRuntimeContext,
                    InitialEpgTypeSnapshotPages);
            }
        }

        std::cout
            << "EPG cache warmup finished: backend="
            << backendRuntimeContext->backendId
            << ", accepted="
            << (result.accepted ? "true" : "false")
            << ", fetched="
            << (result.fetched ? "true" : "false")
            << ", stored="
            << (result.stored ? "true" : "false")
            << ", events="
            << result.eventCount
            << ", genreIndexed="
            << (genreIndexed ? "true" : "false")
            << std::endl;
    }
}
