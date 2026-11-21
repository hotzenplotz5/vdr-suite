#include "BackendPollingCoordinator.h"

void BackendPollingCoordinator::addPollingService(
    const std::string& backendId,
    PollingService& pollingService)
{
    entries_.push_back({backendId, &pollingService});
}

void BackendPollingCoordinator::pollAll()
{
    for (Entry& entry : entries_)
    {
        entry.pollingService->poll();
    }
}

int BackendPollingCoordinator::backendCount() const
{
    return static_cast<int>(entries_.size());
}
