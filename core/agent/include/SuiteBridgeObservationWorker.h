#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeObservationService.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace vdrsuite::agent
{

class SuiteBridgeObservationWorker
{
public:
    explicit SuiteBridgeObservationWorker(
        ISuiteBridgeLocalTransport& transport,
        SuiteBridgeObservationConfig config = {});

    ~SuiteBridgeObservationWorker();

    void start();
    void stop();

    bool running() const;
    SuiteBridgeObservationSnapshot snapshot() const;

private:
    void runLoop();
    void publishSnapshot();

    SuiteBridgeObservationService service_;

    mutable std::mutex mutex_;
    std::condition_variable wakeCondition_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    bool stopRequested_ = false;
    SuiteBridgeObservationSnapshot publishedSnapshot_;
};

}