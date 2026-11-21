#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

struct EpgMetadataMaterializationRequest
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string key;
};

enum class EpgMetadataMaterializationOutcome
{
    Success,
    NotFound,
    TransportFailure
};

class EpgMetadataMaterializationQueue
{
public:
    static EpgMetadataMaterializationQueue& instance()
    {
        static EpgMetadataMaterializationQueue queue;
        return queue;
    }

    bool request(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId)
    {
        if (channelId.empty() || eventId.empty()) return false;

        const std::string normalizedBackendId =
            backendId.empty() ? "default" : backendId;
        const std::string key = makeKey(
            normalizedBackendId,
            channelId,
            eventId);
        const auto now = Clock::now();

        std::lock_guard<std::mutex> lock(mutex_);

        const auto suppressed = suppressedUntilByKey_.find(key);
        if (suppressed != suppressedUntilByKey_.end())
        {
            if (suppressed->second > now) return false;
            suppressedUntilByKey_.erase(suppressed);
        }

        if (pendingKeys_.find(key) != pendingKeys_.end()) return false;
        if (requests_.size() >= MaximumPendingRequests) return false;

        EpgMetadataMaterializationRequest request;
        request.backendId = normalizedBackendId;
        request.channelId = channelId;
        request.eventId = eventId;
        request.key = key;

        pendingKeys_.insert(key);
        requests_.push_back(std::move(request));
        return true;
    }

    std::vector<EpgMetadataMaterializationRequest> take(int maximumRequests)
    {
        const int boundedMaximum = std::max(1, std::min(maximumRequests, 16));
        std::vector<EpgMetadataMaterializationRequest> result;

        std::lock_guard<std::mutex> lock(mutex_);
        while (!requests_.empty() &&
               static_cast<int>(result.size()) < boundedMaximum)
        {
            result.push_back(std::move(requests_.front()));
            requests_.pop_front();
        }
        return result;
    }

    void complete(
        const std::string& key,
        EpgMetadataMaterializationOutcome outcome)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingKeys_.erase(key);

        if (outcome == EpgMetadataMaterializationOutcome::Success)
        {
            suppressedUntilByKey_.erase(key);
            return;
        }

        suppressedUntilByKey_[key] = Clock::now() + (
            outcome == EpgMetadataMaterializationOutcome::NotFound
                ? MetadataNotFoundBackoff
                : MetadataTransportBackoff);
    }

    std::size_t pendingCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pendingKeys_.size();
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        requests_.clear();
        pendingKeys_.clear();
        suppressedUntilByKey_.clear();
    }

private:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t MaximumPendingRequests = 128;
    static constexpr auto MetadataNotFoundBackoff = std::chrono::minutes(5);
    static constexpr auto MetadataTransportBackoff = std::chrono::seconds(30);

    static std::string makeKey(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId)
    {
        return backendId + '\x1f' + channelId + '\x1f' + eventId;
    }

    EpgMetadataMaterializationQueue() = default;

    mutable std::mutex mutex_;
    std::deque<EpgMetadataMaterializationRequest> requests_;
    std::set<std::string> pendingKeys_;
    std::map<std::string, Clock::time_point> suppressedUntilByKey_;
};
