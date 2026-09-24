#pragma once

#include <algorithm>
#include <chrono>
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
    using Clock = std::chrono::steady_clock;

    void request(const std::string& backendId, int attempts = 1)
    {
        if (backendId.empty() || attempts < 1) return;
        std::lock_guard<std::mutex> lock(mutex_);
        pending_[backendId] = std::max(pending_[backendId], attempts);
    }

    std::vector<std::string> takePending(Clock::time_point now = Clock::now())
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> result;
        for (auto it = pending_.begin(); it != pending_.end();) {
            const auto retry = retries_.find(it->first);
            if (retry != retries_.end() && now < retry->second.due) {
                ++it;
                continue;
            }
            result.push_back(it->first);
            if (--it->second == 0) it = pending_.erase(it);
            else ++it;
        }
        return result;
    }

    // A failed read/write is still outstanding work, even if the provider
    // change counter has already been acknowledged by the polling snapshot.
    // Retry reads only, with bounded frequency and one entry per backend.
    void failed(const std::string& backendId, Clock::time_point now = Clock::now())
    {
        if (backendId.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& retry = retries_[backendId];
        retry.delaySeconds = retry.delaySeconds == 0
            ? 5 : std::min(60, retry.delaySeconds * 2);
        retry.due = now + std::chrono::seconds(retry.delaySeconds);
        pending_[backendId] = std::max(pending_[backendId], 1);
    }

    void completed(const std::string& backendId)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        retries_.erase(backendId);
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
    struct Retry {
        int delaySeconds = 0;
        Clock::time_point due;
    };
    std::mutex mutex_;
    std::map<std::string, Retry> retries_;
    std::map<std::string, int> pending_;
    std::set<std::string> completed_;
};
