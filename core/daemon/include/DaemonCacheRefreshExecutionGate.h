#pragma once

#include <mutex>

class DaemonCacheRefreshExecutionGate
{
public:
    using Lease = std::unique_lock<std::mutex>;

    static Lease acquire()
    {
        return Lease(mutex());
    }

private:
    static std::mutex& mutex()
    {
        static std::mutex instance;
        return instance;
    }
};
