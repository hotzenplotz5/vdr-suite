#include "DaemonRuntime.h"

#include "VdrChangeEvent.h"

void DaemonRuntime::publishPendingHbbtvPresentationChanges()
{
    if (!hbbtvPresentationChangedHint_.exchange(false) ||
        hbbtvPresentationChangedBackendId_.empty() ||
        !snapshotChangeFeed_ ||
        !snapshotChangeFeedService_ ||
        !liveTransportService_ ||
        !snapshotCacheService_) {
        return;
    }

    const int previousLatestSequenceNumber =
        snapshotChangeFeed_->latestSequenceNumber();

    snapshotChangeFeedService_->appendChanges(
        *snapshotChangeFeed_,
        snapshotCacheService_->generation(),
        {VdrChangeEvent(VdrChangeType::HbbtvPresentationChanged)},
        hbbtvPresentationChangedBackendId_);

    for (const auto& entry : snapshotChangeFeed_->entries()) {
        if (entry.sequenceNumber() > previousLatestSequenceNumber) {
            liveTransportService_->publishChangeFeedEntry(entry);
        }
    }
}
