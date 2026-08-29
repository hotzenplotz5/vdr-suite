#include "DaemonRuntime.h"

#include "DaemonCacheRefreshExecutionGate.h"
#include "GenreBrowserApiRuntime.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingDuration.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
std::int64_t recordingMetadataEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool recordingMetadataCapabilityAvailable(
    const BackendRuntimeContext& context)
{
    if (!context.suiteBridgeAgentRuntime) {
        return false;
    }

    const vdrsuite::agent::SuiteBridgeEmbeddedAgentHealth health =
        context.suiteBridgeAgentRuntime->health();

    return health.running &&
        health.observation.hasDiscovery &&
        health.observation.discovery.capabilityAvailable(
            "recording-metadata");
}

void runRecordingMetadataEnrichment(
    BackendRuntimeContext& context,
    const std::vector<VdrRecording>& recordings,
    std::atomic<bool>& stopRequested,
    const std::string& reason)
{
    if (!context.recordingMetadataEnrichmentService) {
        return;
    }

    const std::int64_t now = recordingMetadataEpochSeconds();

    const std::size_t queued =
        context.recordingMetadataEnrichmentService->reconcileInventory(
            recordings,
            now);

    if (stopRequested.load() || queued == 0) {
        return;
    }

    if (!recordingMetadataCapabilityAvailable(context)) {
        if (reason != "periodic") {
            std::cout
                << "Recording metadata enrichment deferred: reason="
                << reason
                << ", backend="
                << context.backendId
                << ", queued="
                << queued
                << ", capability=unavailable"
                << std::endl;
        }

        return;
    }

    const int processed =
        context.recordingMetadataEnrichmentService->processBatch(now);

    const VdrRecordingNativeMetadataEnrichmentStatus status =
        context.recordingMetadataEnrichmentService->status();

    std::cout
        << "Recording metadata enrichment finished: reason="
        << reason
        << ", backend="
        << context.backendId
        << ", queued="
        << queued
        << ", processed="
        << processed
        << ", remaining="
        << status.queuedCount
        << std::endl;
}
}

void DaemonRuntime::startRecordingCacheWarmupWorker()
{
    if (recordingCacheWarmupThread_.joinable()) {
        return;
    }

    recordingCacheWarmupStopRequested_.store(false);
    recordingCacheDirtyHint_.store(false);
    recordingCacheActionRefreshAttempts_.store(0);

    if (vdrRecordingCacheRepository_) {
        for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
            if (!backendRuntimeContext) {
                continue;
            }

            const auto startedAt =
                std::chrono::steady_clock::now();

            const bool warmed =
                vdrRecordingCacheRepository_->warmBrowseSnapshotForBackend(
                    backendRuntimeContext->backendId);

            const auto elapsedMilliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startedAt).count();

            std::cout
                << "Recording browse snapshot startup warmup: backend="
                << backendRuntimeContext->backendId
                << ", warmed="
                << (warmed ? "true" : "false")
                << ", elapsedMs="
                << elapsedMilliseconds
                << std::endl;
        }
    }

    recordingCacheWarmupThread_ = std::thread([this]() {
        runRecordingCacheWarmupWorker();
    });
}

void DaemonRuntime::stopRecordingCacheWarmupWorker()
{
    recordingCacheWarmupStopRequested_.store(true);

    if (recordingCacheWarmupThread_.joinable()) {
        recordingCacheWarmupThread_.join();
    }
}

void DaemonRuntime::runRecordingCacheWarmupWorker()
{
    try {
        const int initialDelaySeconds = 1;
        const int dirtyDebounceSeconds = 30;
        const int metadataRefreshSeconds = 60;

        std::cout
            << "Recording cache warmup worker scheduled after "
            << initialDelaySeconds
            << " seconds"
            << std::endl;

        const auto waitForStop = [this](int seconds) -> bool {
            for (int elapsed = 0; elapsed < seconds; ++elapsed) {
                if (recordingCacheWarmupStopRequested_.load()) {
                    return true;
                }

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }

            return recordingCacheWarmupStopRequested_.load();
        };

        if (waitForStop(initialDelaySeconds)) {
            return;
        }

        refreshRecordingCacheForAllBackends("startup");

        auto lastRefresh = std::chrono::steady_clock::now();
        auto lastMetadataRefresh = lastRefresh;

        while (!recordingCacheWarmupStopRequested_.load()) {
            if (waitForStop(1)) {
                return;
            }

            const auto metadataNow = std::chrono::steady_clock::now();

            const auto secondsSinceMetadataRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    metadataNow - lastMetadataRefresh).count();

            if (secondsSinceMetadataRefresh >= metadataRefreshSeconds &&
                vdrRecordingCacheRepository_) {
                auto refreshLease =
                    DaemonCacheRefreshExecutionGate::acquire();
                if (recordingCacheWarmupStopRequested_.load()) {
                    return;
                }

                for (const auto& backendRuntimeContext :
                     backendRuntimeContexts_) {
                    if (recordingCacheWarmupStopRequested_.load()) {
                        return;
                    }

                    if (!backendRuntimeContext ||
                        !backendRuntimeContext
                             ->recordingMetadataEnrichmentService) {
                        continue;
                    }

                    const std::vector<VdrRecording> recordings =
                        vdrRecordingCacheRepository_->findAllForBackend(
                            backendRuntimeContext->backendId);

                    runRecordingMetadataEnrichment(
                        *backendRuntimeContext,
                        recordings,
                        recordingCacheWarmupStopRequested_,
                        "periodic");

                    GenreBrowserApiRuntime::instance()
                        .refreshRecordingIndex(
                            backendRuntimeContext->backendId);
                }

                lastMetadataRefresh = metadataNow;
            }

            int remainingActionRefreshAttempts =
                recordingCacheActionRefreshAttempts_.load();

            while (remainingActionRefreshAttempts > 0 &&
                   !recordingCacheActionRefreshAttempts_.compare_exchange_weak(
                       remainingActionRefreshAttempts,
                       remainingActionRefreshAttempts - 1)) {
            }

            if (remainingActionRefreshAttempts > 0) {
                refreshRecordingCacheForAllBackends(
                    "recording-action-reconcile");
                lastRefresh = std::chrono::steady_clock::now();
                continue;
            }

            if (!recordingCacheDirtyHint_.load()) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const auto secondsSinceLastRefresh =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - lastRefresh).count();

            if (secondsSinceLastRefresh < dirtyDebounceSeconds) {
                continue;
            }

            if (!recordingCacheDirtyHint_.exchange(false)) {
                continue;
            }

            refreshRecordingCacheForAllBackends("event-stream-dirty-hint");
            lastRefresh = std::chrono::steady_clock::now();
        }
    }
    catch (const std::exception& error) {
        std::cerr
            << "Recording cache warmup worker stopped after exception: "
            << error.what()
            << std::endl;
    }
    catch (...) {
        std::cerr
            << "Recording cache warmup worker stopped after unknown exception"
            << std::endl;
    }
}

void DaemonRuntime::refreshRecordingCacheForAllBackends(
    const std::string& reason)
{
    auto refreshLease = DaemonCacheRefreshExecutionGate::acquire();
    if (recordingCacheWarmupStopRequested_.load()) {
        return;
    }

    if (!vdrRecordingCacheRepository_) {
        std::cout
            << "Recording cache warmup skipped: repository unavailable"
            << std::endl;
        return;
    }

    if (backendRuntimeContexts_.empty()) {
        std::cout
            << "Recording cache warmup skipped: no VDR backend configured"
            << std::endl;
        return;
    }

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        if (recordingCacheWarmupStopRequested_.load()) {
            return;
        }

        if (!backendRuntimeContext || !backendRuntimeContext->service) {
            continue;
        }

        std::cout
            << "Recording cache warmup started: reason="
            << reason
            << ", backend="
            << backendRuntimeContext->backendId
            << std::endl;

        vdrRecordingCacheRepository_->markRefreshStarted(
            backendRuntimeContext->backendId);

        try {
            std::vector<VdrRecording> recordings =
                backendRuntimeContext->service->getRecordings();

            vdrsuite::recording::normalizeForCatalog(recordings);

            const bool stored =
                vdrRecordingCacheRepository_->replaceRecordingsForBackend(
                    backendRuntimeContext->backendId,
                    recordings);

            bool genreIndexed = false;
            if (stored) {
                vdrRecordingCacheRepository_->markRefreshFinished(
                    backendRuntimeContext->backendId,
                    static_cast<int>(recordings.size()));

                if (snapshotCacheService_) {
                    snapshotCacheService_->updateRecordingsForBackend(
                        backendRuntimeContext->backendId,
                        recordings);
                }

                runRecordingMetadataEnrichment(
                    *backendRuntimeContext,
                    recordings,
                    recordingCacheWarmupStopRequested_,
                    reason);

                genreIndexed = GenreBrowserApiRuntime::instance()
                    .refreshRecordingIndex(
                        backendRuntimeContext->backendId);
            }
            else {
                vdrRecordingCacheRepository_->markRefreshFailed(
                    backendRuntimeContext->backendId,
                    "repository replace failed");
            }

            std::cout
                << "Recording cache warmup finished: backend="
                << backendRuntimeContext->backendId
                << ", stored="
                << (stored ? "true" : "false")
                << ", recordings="
                << recordings.size()
                << ", genreIndexed="
                << (genreIndexed ? "true" : "false")
                << std::endl;
        }
        catch (const std::exception& error) {
            vdrRecordingCacheRepository_->markRefreshFailed(
                backendRuntimeContext->backendId,
                error.what());

            std::cerr
                << "Recording cache warmup failed: backend="
                << backendRuntimeContext->backendId
                << ", error="
                << error.what()
                << std::endl;
        }
        catch (...) {
            vdrRecordingCacheRepository_->markRefreshFailed(
                backendRuntimeContext->backendId,
                "unknown exception");

            std::cerr
                << "Recording cache warmup failed after unknown exception: backend="
                << backendRuntimeContext->backendId
                << std::endl;
        }
    }
}
