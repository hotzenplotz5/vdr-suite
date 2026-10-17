#pragma once
#include "LiveUpdateEvent.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// Device subscription buffers only change hints from the existing Suite
// event transport; admission is performed by SecurityHttpGate.
class DeviceLiveEventSubscription {
public:
    DeviceLiveEventSubscription(std::string backend,
                                std::vector<std::string> domains,
                                std::string epoch)
        : backend_(std::move(backend)), domains_(std::move(domains)),
          epoch_(std::move(epoch)),
          expires_(std::chrono::steady_clock::now() +
                   std::chrono::minutes(2))
    {
        std::ostringstream json;
        json << "{\"backendId\":\"" << backend_
             << "\",\"daemonEpoch\":\"" << epoch_
             << "\",\"subscriptionSequence\":0,\"authorizedDomains\":[";
        for (std::size_t i = 0; i < domains_.size(); ++i) {
            if (i) json << ",";
            json << "\"" << domains_[i] << "\"";
        }
        json << "]}";
        pending_.push_back("event: hello\ndata: " + json.str() + "\n\n");
    }

    bool next(std::string& frame, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait_for(lock, timeout, [this]() {
            return closed_ || !pending_.empty();
        });
        if (std::chrono::steady_clock::now() >= expires_) {
            closed_ = true;
            pending_.clear();
        }
        if (pending_.empty()) {
            frame.clear();
            return !closed_;
        }
        frame = std::move(pending_.front());
        pending_.pop_front();
        return true;
    }

    void deliver(const LiveUpdateEvent& event)
    {
        if (event.backendId() != backend_) return;
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || std::chrono::steady_clock::now() >= expires_) return;
        bool timers = false, recordings = false;
        for (const auto& domain : event.changedDomains()) {
            if (domain == "timers" && hasDomain("timers")) timers = true;
            if ((domain == "recordings" || domain == "recordingMarks") &&
                hasDomain("recordings")) recordings = true;
        }
        if (!timers && !recordings) return;
        if (pending_.size() >= 64) {
            pending_.clear();
            pending_.push_back("event: resync\ndata: {}\n\n");
            closed_ = true;
            condition_.notify_one();
            return;
        }
        ++sequence_;
        std::ostringstream json;
        json << "{\"backendId\":\"" << backend_
             << "\",\"daemonEpoch\":\"" << epoch_
             << "\",\"subscriptionSequence\":" << sequence_
             << ",\"changedDomains\":[";
        if (timers) json << "\"timers\"";
        if (recordings) {
            if (timers) json << ",";
            json << "\"recordings\"";
        }
        json << "]}";
        pending_.push_back("event: update\nid: " +
            std::to_string(sequence_) + "\ndata: " + json.str() + "\n\n");
        condition_.notify_one();
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        pending_.clear();
        condition_.notify_one();
    }
private:
    bool hasDomain(const std::string& domain) const {
        return std::find(domains_.begin(), domains_.end(), domain) !=
            domains_.end();
    }
    std::string backend_;
    std::vector<std::string> domains_;
    std::string epoch_;
    std::chrono::steady_clock::time_point expires_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> pending_;
    std::uint64_t sequence_ = 0;
    bool closed_ = false;
};

class DeviceLiveEventHub {
public:
    DeviceLiveEventHub() {
        std::random_device random;
        std::ostringstream value;
        value << "daemon_";
        for (int i = 0; i < 4; ++i)
            value << std::hex << std::setfill('0') << std::setw(8)
                  << static_cast<std::uint32_t>(random());
        epoch_ = value.str();
    }

    std::shared_ptr<DeviceLiveEventSubscription> subscribe(
        const std::string& backend,
        const std::vector<std::string>& domains)
    {
        auto subscriber = std::make_shared<DeviceLiveEventSubscription>(
            backend, domains, epoch_);
        std::lock_guard<std::mutex> lock(mutex_);
        subscriptions_.erase(
            std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                [](const std::weak_ptr<DeviceLiveEventSubscription>& weak) {
                    return weak.expired();
                }), subscriptions_.end());
        if (subscriptions_.size() >= 8) return nullptr;
        subscriptions_.push_back(subscriber);
        return subscriber;
    }

    void publish(const LiveUpdateEvent& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = subscriptions_.begin(); it != subscriptions_.end();) {
            auto subscriber = it->lock();
            if (!subscriber) it = subscriptions_.erase(it);
            else { subscriber->deliver(event); ++it; }
        }
    }

    void disconnectAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& weak : subscriptions_) {
            if (auto subscriber = weak.lock()) subscriber->close();
        }
        subscriptions_.clear();
    }
private:
    std::string epoch_;
    std::mutex mutex_;
    std::vector<std::weak_ptr<DeviceLiveEventSubscription>> subscriptions_;
};
