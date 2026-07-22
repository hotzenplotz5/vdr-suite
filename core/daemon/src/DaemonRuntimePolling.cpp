#include "DaemonRuntime.h"

void DaemonRuntime::pollVdrAndUpdateChangeFeed()
{
    backendPollingCoordinator_->pollAll();

    for (const auto& backendRuntimeContext : backendRuntimeContexts_) {
        const int previousLatestSequenceNumber =
            snapshotChangeFeed_->latestSequenceNumber();

        snapshotChangeFeedService_->appendChanges(
            *snapshotChangeFeed_,
            snapshotCacheService_->generation(),
            backendRuntimeContext->pollingService->changeEvents(),
            backendRuntimeContext->backendId);

        for (const auto& entry : snapshotChangeFeed_->entries()) {
            if (entry.sequenceNumber() > previousLatestSequenceNumber) {
                liveTransportService_->publishChangeFeedEntry(entry);
            }
        }
    }
}
