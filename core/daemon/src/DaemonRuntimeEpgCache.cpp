#include "DaemonRuntime.h"

#include "VdrEventQuery.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

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

        while (!epgCacheWarmupStopRequested_.load()) {
            if (waitForStop(5)) {
                return;
            }

            if (!epgCacheDirtyHint_.load()) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
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
    query.timespan = 172800;
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
            << std::endl;
    }
}
