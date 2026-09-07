#pragma once

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

// Hints are coalesced per backend. Taking work never consumes hints arriving
// during the provider read. Only successful cache writes become feed work.
class RecordingCacheRefreshQueue
{
public:
    void request(const std::string& backendId, int attempts = 1)
    {
        if (backendId.empty() || attempts < 1) return;
        std::lock_guard<std::mutex> lock(mutex_);
        pending_[backendId] = std::max(pending_[backendId], attempts);
    }

    std::vector<std::string> takePending()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result;
        for (auto it = pending_.begin(); it != pending_.end();) {
            result.push_back(it->first);
            if (--it->second == 0) it = pending_.erase(it);
            else ++it;
        }
        return result;
    }

    void completed(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        completed_.insert(backendId);
    }

    std::set<std::string> takeCompleted()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::set<std::string> result;
        result.swap(completed_);
        return result;
    }

private:
    std::mutex mutex_;
    std::map<std::string, int> pending_;
    std::set<std::string> completed_;
};
