#include "DaemonRuntime.h"

#include "GenreBrowserApiRuntime.h"
#include "VdrEventQuery.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>

namespace
{
constexpr std::int64_t GenreWindowSeconds = 48 * 60 * 60;

std::int64_t epgGenreEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

void DaemonRuntime::startEpgCacheWarmupWorker()
{
    if (epgCacheWarmupThread_.joinable()) {
        return;
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
        const int genreRefreshSeconds = 60;

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
        auto lastGenreRefresh = lastRefresh;

        while (!epgCacheWarmupStopRequested_.load()) {
            if (waitForStop(5)) {
                return;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto secondsSinceGenreRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastGenreRefresh).count();

            if (secondsSinceGenreRefresh >= genreRefreshSeconds) {
                const std::int64_t fromTime = epgGenreEpochSeconds();
                for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
                    if (epgCacheWarmupStopRequested_.load()) {
                        return;
                    }
                    if (!backendRuntimeContext) {
                        continue;
                    }
                    GenreBrowserApiRuntime::instance().refreshEpgIndex(
                        backendRuntimeContext->backendId,
                        fromTime,
                        fromTime + GenreWindowSeconds,
                        8);
                }
                lastGenreRefresh = std::chrono::steady_clock::now();
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
            lastRefresh = std::chrono::steady_clock::now();
            lastGenreRefresh = lastRefresh;
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
    if (backendRuntimeContexts_.empty()) {
        std::cout
            << "EPG cache warmup skipped: no VDR backend configured"
            << std::endl;
        return;
    }

    VdrEventQuery query;
    query.from = -1;
    query.timespan = static_cast<int>(GenreWindowSeconds);
    query.channelEventLimit = 160;

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (epgCacheWarmupStopRequested_.load()) {
            return;
        }

        if (!backendRuntimeContext || !backendRuntimeContext->epgCacheService) {
            continue;
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
                32);
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
