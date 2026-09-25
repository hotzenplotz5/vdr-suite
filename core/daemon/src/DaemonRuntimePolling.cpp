#include "DaemonRuntime.h"

#include <chrono>

void DaemonRuntime::pollVdrAndUpdateChangeFeed()
{
    backendPollingCoordinator_->pollAll();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        const int previousLatestSequenceNumber =
            snapshotChangeFeed_->latestSequenceNumber();

        std::vector<VdrChangeEvent> readyChanges;
        for (const auto& change : backendRuntimeContext->pollingService->changeEvents()) {
            if (change.type() == VdrChangeType::RecordingsChanged) {
                // Polling is also the recovery path when backend SSE is absent.
                // Publish recordings only after the folder cache has committed.
                recordingCacheRefreshQueue_.request(backendRuntimeContext->backendId);
            } else {
                readyChanges.push_back(change);
            }
        }

        if (backendRuntimeContext->suiteBridgeAgentRuntime) {
            const auto health =
                backendRuntimeContext->suiteBridgeAgentRuntime->health();
            const auto& observation = health.observation;

            if (embeddedBackendLifecycleService_) {
                const std::int64_t lifecycleNow =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now()
                            .time_since_epoch()).count();
                const bool healthy =
                    health.running &&
                    observation.state ==
                        vdrsuite::agent::SuiteBridgeObservationState::SnapshotCurrent;
                embeddedBackendLifecycleService_->maintainBackend(
                    backendRuntimeContext->backendId,
                    healthy,
                    lifecycleNow);
            }

            if (observation.hasBaseline) {
                const bool snapshotCurrent =
                    observation.state ==
                    vdrsuite::agent::SuiteBridgeObservationState::SnapshotCurrent;

                if (backendRuntimeContext->recordingListChangeTracker.observe(
                        observation.baseline.counterEpoch,
                        observation.baseline.recordingList,
                        snapshotCurrent,
                        observation.baseline.counterOverflow)) {
                    // SuiteBridge is an additional native dirty hint only.
                    // The cache worker remains the sole inventory reader/writer,
                    // and completion is published only after a successful commit.
                    recordingCacheRefreshQueue_.request(
                        backendRuntimeContext->backendId);
                }

                if (backendRuntimeContext->recordingMarksChangeTracker.observe(
                        observation.baseline.counterEpoch,
                        observation.baseline.marksModified,
                        snapshotCurrent,
                        observation.baseline.counterOverflow)) {
                    readyChanges.emplace_back(
                        VdrChangeType::RecordingMarksChanged);
                }
            }
        }

        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_,
            snapshotCacheService_->generation(),
            readyChanges,
            backendRuntimeContext->backendId);

        for (const auto& entry : snapshotChangeFeed_->entries()) {
            if (entry.sequenceNumber() > previousLatestSequenceNumber) {
                liveTransportService_->publishChangeFeedEntry(entry);
            }
        }
    }
}
