#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <utility>

// Queue-only browser SSE delivery. The Snapshot Change Feed remains the
// authority; subscribers hold bounded, disposable transport frames only.
class BrowserLiveEventSubscription {
public:
    static constexpr std::size_t maxPendingFrames = 64;
    static constexpr std::size_t maxPendingBytes = 1024 * 1024;

    explicit BrowserLiveEventSubscription(std::string initial)
        : expires_(std::chrono::steady_clock::now() + std::chrono::minutes(2))
    {
        if (!initial.empty() && initial.size() <= maxPendingBytes) {
            pendingBytes_ = initial.size();
            pending_.push_back(std::move(initial));
        }
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
            pendingBytes_ = 0;
        }
        if (pending_.empty()) {
            frame.clear();
            return !closed_;
        }
        frame = std::move(pending_.front());
        pendingBytes_ -= frame.size();
        pending_.pop_front();
        return true;
    }

    void deliver(const std::string& frame, const std::string& resync)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || std::chrono::steady_clock::now() >= expires_) return;
        if (frame.size() > maxPendingBytes ||
            pending_.size() >= maxPendingFrames ||
            pendingBytes_ > maxPendingBytes - frame.size()) {
            resyncAndCloseLocked(resync);
            return;
        }
        pendingBytes_ += frame.size();
        pending_.push_back(frame);
        condition_.notify_one();
    }

    void resyncAndClose(const std::string& frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) return;
        resyncAndCloseLocked(frame);
    }

    void close()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        pending_.clear();
        pendingBytes_ = 0;
        condition_.notify_one();
    }

private:
    void resyncAndCloseLocked(const std::string& frame)
    {
        pending_.clear();
        pendingBytes_ = 0;
        if (frame.size() <= maxPendingBytes) {
            pendingBytes_ = frame.size();
            pending_.push_back(frame);
        }
        closed_ = true;
        condition_.notify_one();
    }

    std::chrono::steady_clock::time_point expires_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<std::string> pending_;
    std::size_t pendingBytes_ = 0;
    bool closed_ = false;
};
