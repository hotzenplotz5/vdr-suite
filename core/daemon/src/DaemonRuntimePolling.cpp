#include "DaemonRuntime.h"

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
