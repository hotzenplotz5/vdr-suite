#include "SuiteBridgeObservationWorker.h"

namespace vdrsuite::agent
{

SuiteBridgeObservationWorker::SuiteBridgeObservationWorker(
    ISuiteBridgeLocalTransport& transport,
    SuiteBridgeObservationConfig config)
    : service_(transport, std::move(config)),
      publishedSnapshot_(service_.snapshot())
{
}

SuiteBridgeObservationWorker::~SuiteBridgeObservationWorker()
{
    stop();
}

void SuiteBridgeObservationWorker::start()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (thread_.joinable())
    {
        return;
    }

    stopRequested_ = false;
    service_.start(SuiteBridgeObservationClock::now());
    publishedSnapshot_ = service_.snapshot();
    running_.store(true);
    thread_ = std::thread(
        &SuiteBridgeObservationWorker::runLoop,
        this);
}

void SuiteBridgeObservationWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!thread_.joinable())
        {
            running_.store(false);
            return;
        }

        stopRequested_ = true;
        wakeCondition_.notify_all();
    }

    thread_.join();
}

bool SuiteBridgeObservationWorker::running() const
{
    return running_.load();
}

SuiteBridgeObservationSnapshot
SuiteBridgeObservationWorker::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return publishedSnapshot_;
}

void SuiteBridgeObservationWorker::runLoop()
{
    while (true)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (stopRequested_)
            {
                break;
            }
        }

        const SuiteBridgeObservationTimePoint now =
            SuiteBridgeObservationClock::now();

        service_.refresh(now);

        if (service_.attemptDue(now))
        {
            service_.attempt(now);
        }

        publishSnapshot();

        const SuiteBridgeObservationTimePoint afterAttempt =
            SuiteBridgeObservationClock::now();
        service_.refresh(afterAttempt);
        publishSnapshot();

        const auto nextWake = service_.nextWakeAt(afterAttempt);

        std::unique_lock<std::mutex> lock(mutex_);

        if (stopRequested_)
        {
            break;
        }

        if (nextWake)
        {
            wakeCondition_.wait_until(
                lock,
                *nextWake,
                [this]() {
                    return stopRequested_;
                });
        }
        else
        {
            wakeCondition_.wait(
                lock,
                [this]() {
                    return stopRequested_;
                });
        }
    }

    service_.stop(SuiteBridgeObservationClock::now());
    publishSnapshot();
    running_.store(false);
}

void SuiteBridgeObservationWorker::publishSnapshot()
{
    const SuiteBridgeObservationSnapshot current = service_.snapshot();
    std::lock_guard<std::mutex> lock(mutex_);
    publishedSnapshot_ = current;
}

}