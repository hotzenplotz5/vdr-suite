#include "DaemonRuntime.h"

#include "DaemonCacheRefreshExecutionGate.h"
#include "GenreBrowserApiRuntime.h"
#include "VdrRecordingCacheRepository.h"
#include "VdrRecordingDuration.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaImport.h"
#include "MediaImportRecordingEvidence.h"
#include "MediaToolsAgentExecutor.h"
#include <filesystem>

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::int64_t recordingMetadataEpochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// MT.META6: consume the already-committed import snapshot on the EXISTING
// VDR cache refresh, AFTER the normal readback established Core success.
// This is not a worker, a second scraper, or another media operation.
// A missing/failed metadata assignment never rolls back a verified recording.
bool assignFrozenMetadataAfterImport(
    const std::string& backendId,
    const std::vector<VdrRecording>& recordings,
    vdrsuite::operations::MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    MetadataRepository& metadata)
{
    using State = vdrsuite::operations::MutationOperationState;
    const auto succeeded = operations.listByActionFamilyAndStates(
        "media.import", {State::succeeded}, 256);
    if (!succeeded.ok()) return false;
    bool changed = false;
    for (const auto& op : succeeded.operations) {
        if (op.backendId != backendId || op.resourceType != "MediaImportSource")
            continue;
        const auto assignment = commands.findAssignmentForOperation(
            backendId, op.operationId,
            vdrsuite::mediatools::kMediaImportCommandType);
        if (!assignment || assignment->operationId != op.operationId ||
            assignment->backendGeneration != op.backendGeneration ||
            assignment->backendId != backendId ||
            assignment->commandType != vdrsuite::mediatools::kMediaImportCommandType)
            continue;
        const std::string frozen =
            vdrsuite::mediatools::mediaImportSelectionMetadata(
                assignment->payload);
        if (frozen.compare(0, 3, "v2|") != 0) continue;
        vdrsuite::mediatools::ImportConfirmedMetadata selected;
        if (!vdrsuite::mediatools::parseFrozenImportMetadata(frozen, selected) ||
            selected.externalId.empty())
            continue;
        const auto result = commands.resultForCommand(assignment->commandId);
        std::string proof, sourceProof;
        if (!result || result->commandId != assignment->commandId ||
            result->backendId != backendId ||
            result->backendGeneration != op.backendGeneration ||
            result->requestFingerprint != assignment->requestFingerprint ||
            result->claimEpoch != assignment->claimEpoch ||
            result->dispatchState != "accepted_by_executor" ||
            !vdrsuite::mediatools::parseMediaImportResultEvidence(
                result->resultEvidence, proof, sourceProof))
            continue;

        // Match the *proven* native .rec, not a browser-supplied folder,
        // a fuzzy filename, or "latest recording". Native providers may
        // report the symlink /srv/vdr/video rather than video.00.
        const VdrRecording* matched = nullptr;
        bool ambiguous = false;
        for (const auto& rec : recordings) {
            if (rec.backendId != backendId || rec.backendNativeId.empty() ||
                rec.title.empty() || rec.id.empty())
                continue;
            using vdrsuite::mediatools::recordingPathEvidence;
            bool same = recordingPathEvidence(rec.backendNativeId) == proof ||
                recordingPathEvidence(rec.path) == proof;
            if (!same) {
                const std::filesystem::path native(rec.backendNativeId);
                if (native.is_absolute() && native.extension() == ".rec" &&
                    native.lexically_normal() == native) {
                    std::error_code error;
                    const auto canonical =
                        std::filesystem::canonical(native, error);
                    same = !error &&
                        recordingPathEvidence(canonical.string()) == proof;
                }
            }
            if (!same) continue;
            if (matched != nullptr) {
                ambiguous = true;
                break;
            }
            matched = &rec;
        }
        if (ambiguous || matched == nullptr) continue;

        // Respect every existing manual choice; never supersede it during
        // cache refresh. Repeated refreshes are idempotent after assignment.
        const auto existing = metadata.getManualRecordingMetadataReadOnly(
            backendId, matched->backendNativeId);
        if (existing.found) continue;

        ManualRecordingMetadataSelection selection;
        selection.backendId = backendId;
        selection.resourceKey = matched->backendNativeId;
        selection.providerId = "tmdb";
        selection.externalNamespace = selected.externalNamespace;
        selection.externalId = selected.externalId;
        selection.mediaType = selected.mediaType;
        selection.title = selected.title;
        selection.overview = selected.overview;
        selection.posterReference = selected.posterReference;
        selection.seasonNumber = selected.seasonNumber;
        selection.episodeNumber = selected.episodeNumber;
        selection.actorRef = op.actorId;
        ManualRecordingMetadataAssignment saved;
        if (metadata.assignManualRecordingMetadata(selection, saved)) {
            changed = true;
            std::cout << "Media Import TMDb assignment saved: backend="
                      << backendId << ", operation=" << op.operationId
                      << ", nativeId=" << matched->backendNativeId
                      << std::endl;
        } else {
            // This may reflect unavailable artwork or a transient DB error.
            // No replayed import and no blocked video action.
            std::cerr << "Media Import TMDb assignment pending: backend="
                      << backendId << ", operation=" << op.operationId
                      << std::endl;
        }
    }
    return changed;
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

bool runRecordingMetadataEnrichment(
    BackendRuntimeContext& context,
    const std::vector<VdrRecording>& recordings,
    std::atomic<bool>& stopRequested,
    const std::string& reason)
{
    if (!context.recordingMetadataEnrichmentService) {
        return false;
    }

    const std::int64_t now = recordingMetadataEpochSeconds();

    const std::size_t queued =
        context.recordingMetadataEnrichmentService->reconcileInventory(
            recordings,
            now);

    if (stopRequested.load() || queued == 0) {
        return false;
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

        return false;
    }

    const VdrRecordingNativeMetadataEnrichmentStatus before =
        context.recordingMetadataEnrichmentService->status();

    const int processed =
        context.recordingMetadataEnrichmentService->processBatch(now);

    const VdrRecordingNativeMetadataEnrichmentStatus status =
        context.recordingMetadataEnrichmentService->status();

    const bool presentationChanged =
        status.resolvedFound > before.resolvedFound ||
        status.resolvedNotFound > before.resolvedNotFound;

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
        << ", presentationChanged="
        << (presentationChanged ? "true" : "false")
        << std::endl;

    return presentationChanged;
}
}

void DaemonRuntime::startRecordingCacheWarmupWorker()
{
    if (recordingCacheWarmupThread_.joinable()) {
        return;
    }

    recordingCacheWarmupStopRequested_.store(false);

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

        auto lastMetadataRefresh = std::chrono::steady_clock::now();

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

                    if (runRecordingMetadataEnrichment(
                            *backendRuntimeContext,
                            recordings,
                            recordingCacheWarmupStopRequested_,
                            "periodic")) {
                        recordingPresentationChangeQueue_.request(
                            backendRuntimeContext->backendId);
                    }

                    GenreBrowserApiRuntime::instance()
                        .refreshRecordingIndex(
                            backendRuntimeContext->backendId);
                }

                lastMetadataRefresh = metadataNow;
            }

            for (const auto& backendId : recordingCacheRefreshQueue_.takePending()) {
                refreshRecordingCacheForAllBackends(
                    "event-stream-dirty-hint", backendId);
            }
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
    const std::string& reason, const std::string& backendId)
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

        if (!backendRuntimeContext || !backendRuntimeContext->service ||
            (!backendId.empty() && backendRuntimeContext->backendId != backendId)) {
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
                const bool ready = vdrRecordingCacheRepository_->markRefreshFinished(
                    backendRuntimeContext->backendId,
                    static_cast<int>(recordings.size()));
                if (ready && mediaImportOperationFence_)
                {
                    // Only a successfully committed, fresh native provider
                    // inventory can verify an unverified import operation.
                    const auto cacheStatus =
                        vdrRecordingCacheRepository_->statusForBackend(
                            backendRuntimeContext->backendId);
                    const auto committedRecordings =
                        vdrRecordingCacheRepository_->findAllForBackend(
                            backendRuntimeContext->backendId);
                    const auto verified =
                        mediaImportOperationFence_->reconcileRecordingCache(
                            backendRuntimeContext->backendId, cacheStatus,
                            committedRecordings, recordingMetadataEpochSeconds());
                    if (verified != 0)
                        std::cout << "Media Import readback verified: backend="
                                  << backendRuntimeContext->backendId
                                  << ", operations=" << verified << std::endl;
                }
                if (ready && metadataRepository_ &&
                    mutationOperationRepository_ &&
                    backendAgentCommandRepository_) {
                    // Includes a newly succeeded operation AND one recovered
                    // after daemon restart. The original provider response
                    // remains in the existing immutable Operation payload.
                    const auto nativeRecordings =
                        vdrRecordingCacheRepository_->findAllForBackend(
                            backendRuntimeContext->backendId);
                    if (assignFrozenMetadataAfterImport(
                            backendRuntimeContext->backendId,
                            nativeRecordings,
                            *mutationOperationRepository_,
                            *backendAgentCommandRepository_,
                            *metadataRepository_))
                        recordingPresentationChangeQueue_.request(
                            backendRuntimeContext->backendId);
                }
                // The existing native Recording refresh is the independent
                // evidence for a completed HEVC Publish. A private candidate,
                // an Agent result and a new VDR Recording are three different
                // states; only a genuinely refreshed provider inventory can
                // complete the Core operation.
                if (ready && mutationOperationRepository_ &&
                    backendAgentCommandRepository_ &&
                    mediaShrinkReviewRepository_ &&
                    mediaShrinkPlanRuntime_ && publicRecordingIdentities_) {
                    using State = vdrsuite::operations::MutationOperationState;
                    const auto pending =
                        mutationOperationRepository_->listByActionFamilyAndStates(
                            "media.shrink.publish", {State::outcomeUnknown}, 128);
                    if (pending.ok()) {
                        const auto nativeRecordings =
                            vdrRecordingCacheRepository_->findAllForBackend(
                                backendRuntimeContext->backendId);
                        for (const auto& op : pending.operations) {
                            if (op.backendId != backendRuntimeContext->backendId ||
                                op.resourceType != "MediaShrinkReview" ||
                                !vdrsuite::mediatools::shrinkOpaqueId(
                                    op.resourceId, "shk_") ||
                                op.resultReference.rfind("agent.media.shrink:", 0) != 0)
                                continue;
                            const auto commandId = op.resultReference.substr(19);
                            const auto assignment =
                                backendAgentCommandRepository_->findAssignment(
                                    commandId);
                            const auto result =
                                backendAgentCommandRepository_->resultForCommand(
                                    commandId);
                            if (!assignment || !result ||
                                assignment->operationId != op.operationId ||
                                assignment->backendId != op.backendId ||
                                assignment->backendGeneration != op.backendGeneration ||
                                assignment->commandType !=
                                    vdrsuite::mediatools::kMediaShrinkPublishCommandType ||
                                result->commandId != assignment->commandId ||
                                result->resultCategory != "outcome_unknown" ||
                                result->dispatchState != "accepted_by_executor" ||
                                result->resultEvidence != op.resourceId)
                                continue;
                            const auto review = mediaShrinkReviewRepository_->find(
                                op.actorId, op.backendId, op.resourceId);
                            if (!review.ok() || review.review.state != "publishing" ||
                                review.review.revision != 2 ||
                                review.review.publishOperationId != op.operationId ||
                                review.review.backendGeneration !=
                                    op.backendGeneration ||
                                review.review.sourceFingerprint !=
                                    op.expectedResourceFingerprint)
                                continue;
                            const auto source = mediaShrinkPlanRuntime_->
                                trustedSourcePaths(op.backendId,
                                    review.review.recordingId,
                                    op.expectedResourceFingerprint);
                            if (!source) continue;
                            // Mirror the native No-Replace target from
                            // publishShrinkCandidate, never an Agent path.
                            namespace fs = std::filesystem;
                            const fs::path publishedRelative =
                                fs::path(source->relativeRecording.parent_path()
                                    .string() + "_HEVC") /
                                source->relativeRecording.filename();
                            const fs::path publishedNative =
                                source->videoRoot / publishedRelative;
                            std::string matchedNative;
                            unsigned matches = 0;
                            for (const auto& recording : nativeRecordings) {
                                if (recording.backendId == op.backendId &&
                                    fs::path(recording.path) == publishedNative &&
                                    !recording.backendNativeId.empty() &&
                                    !recording.title.empty()) {
                                    ++matches;
                                    matchedNative = recording.backendNativeId;
                                }
                            }
                            if (matches != 1) continue;
                            try {
                                const auto published = vdrsuite::mediatools::
                                    inspectShrink(source->videoRoot,
                                                  publishedRelative);
                                if (published.segments.empty()) continue;
                                vdrsuite::mediatools::NativeProcessRunner probe(
                                    std::chrono::seconds(120));
                                const auto codec = probe.run({
                                    "/usr/bin/ffprobe", "-v", "error",
                                    "-select_streams", "v:0",
                                    "-show_entries", "stream=codec_name",
                                    "-of", "default=nw=1:nk=1",
                                    (publishedNative / "00001.ts").string()
                                }, true);
                                if (codec.exitCode != 0 ||
                                    (codec.output != "hevc\n" &&
                                     codec.output != "hevc\r\n"))
                                    continue;
                                const auto publicId = publicRecordingIdentities_->
                                    resolveOrCreate(op.backendId, matchedNative);
                                if (!publicId ||
                                    !vdrsuite::mediatools::shrinkOpaqueId(
                                        *publicId, "rec_"))
                                    continue;
                                const auto transition =
                                    mutationOperationRepository_->transition(
                                        op.operationId, op.operationRevision,
                                        op.state, State::succeeded,
                                        *publicId, recordingMetadataEpochSeconds());
                                if (!transition.ok()) continue;
                                const auto completed =
                                    mediaShrinkReviewRepository_->completePublication(
                                        op.actorId, op.backendId,
                                        op.resourceId, 2);
                                if (completed.ok())
                                    std::cout << "HEVC Publish reconciled: backend="
                                              << op.backendId << std::endl;
                            } catch (...) {
                                // Keep outcomeUnknown for later independent
                                // readback; never re-dispatch the mutation.
                            }
                        }
                    }
                }
                recordingCacheRefreshQueue_.completed(backendRuntimeContext->backendId);

                if (runRecordingMetadataEnrichment(
                        *backendRuntimeContext,
                        recordings,
                        recordingCacheWarmupStopRequested_,
                        reason)) {
                    recordingPresentationChangeQueue_.request(
                        backendRuntimeContext->backendId);
                }

                genreIndexed = GenreBrowserApiRuntime::instance()
                    .refreshRecordingIndex(
                        backendRuntimeContext->backendId);
            }
            else {
                recordingCacheRefreshQueue_.failed(backendRuntimeContext->backendId);
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
            recordingCacheRefreshQueue_.failed(backendRuntimeContext->backendId);
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
            recordingCacheRefreshQueue_.failed(backendRuntimeContext->backendId);
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

void DaemonRuntime::publishCompletedRecordingRefreshes()
{
    if (!snapshotChangeFeed_ || !snapshotChangeFeedService_ ||
        !liveTransportService_ || !snapshotCacheService_) return;

    // Feed and transport remain owned by the HTTP/polling thread.
    for (const auto& backendId : recordingCacheRefreshQueue_.takeCompleted()) {
        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_, snapshotCacheService_->generation(),
            {VdrChangeEvent(VdrChangeType::RecordingsChanged)}, backendId);
        liveTransportService_->publishChangeFeedEntry(
            snapshotChangeFeed_->entries().back());
    }
}


void DaemonRuntime::publishRecordingPresentationChanges()
{
    if (!snapshotChangeFeed_ || !snapshotChangeFeedService_ ||
        !liveTransportService_ || !snapshotCacheService_) return;

    // Manual metadata commit already changed the canonical Recording read
    // presentation. Do not reread VDR or rewrite the Recording cache here:
    // publish only the existing backend-scoped recordings invalidation.
    for (const auto& backendId :
         recordingPresentationChangeQueue_.takePending()) {
        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_,
            snapshotCacheService_->generation(),
            {VdrChangeEvent(VdrChangeType::RecordingsChanged)},
            backendId);
        liveTransportService_->publishChangeFeedEntry(
            snapshotChangeFeed_->entries().back());
    }
}
