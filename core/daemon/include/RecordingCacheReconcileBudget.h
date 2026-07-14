#pragma once

#include <atomic>


class RecordingCacheReconcileBudget
{
public:
    static constexpr int maximumAttempts = 2;

    explicit RecordingCacheReconcileBudget(int initialValue = 0)
        : attempts_(bounded(initialValue))
    {
    }

    void store(int value)
    {
        attempts_.store(bounded(value));
    }

    int load() const
    {
        return attempts_.load();
    }

    bool compare_exchange_weak(
        int& expected,
        int desired)
    {
        return attempts_.compare_exchange_weak(
            expected,
            bounded(desired));
    }

private:
    static int bounded(int value)
    {
        if (value < 0)
        {
            return 0;
        }

        if (value > maximumAttempts)
        {
            return maximumAttempts;
        }

        return value;
    }

    std::atomic<int> attempts_;
};
