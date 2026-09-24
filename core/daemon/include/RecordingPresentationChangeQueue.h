#pragma once

#include <mutex>
#include <set>
#include <string>

// Thread-safe handoff from committed Recording presentation mutations to the
// daemon-owned SnapshotChangeFeed publisher. This queue owns no metadata or
// Recording state; it only coalesces backend-scoped invalidation hints.
class RecordingPresentationChangeQueue final
{
public:
    void request(const std::string& backendId)
    {
        if (backendId.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.insert(backendId);
    }

    std::set<std::string> takePending()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::set<std::string> result;
        result.swap(pending_);
        return result;
    }

private:
    std::mutex mutex_;
    std::set<std::string> pending_;
};
