#include "Database.h"
#include "EpgArtworkEnrichmentService.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>

class FakeResolver final : public IEpgArtworkResolver
{
public:
    std::atomic<int> calls{0};

    EpgArtworkResolution resolve(
        const std::string&,
        const VdrEvent& event) override
    {
        const int callNumber = ++calls;
        if (callNumber == 1)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            firstCallEntered_ = true;
            changed_.notify_all();
            changed_.wait(lock, [this]() { return firstCallReleased_; });
        }

        EpgArtworkResolution result;
        if (event.id == "unavailable")
        {
            return result;
        }

        result.attempted = true;
        if (event.id == "missing")
        {
            return result;
        }

        result.found = true;
        result.artwork.provider = "tvscraper";
        result.artwork.path = "/cache/" + event.id + ".jpg";
        result.artwork.width = 1280;
        result.artwork.height = 720;
        result.artwork.resolvedAt = 1234;
        return result;
    }

    bool waitUntilFirstCallEntered(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return changed_.wait_for(lock, timeout, [this]() {
            return firstCallEntered_;
        });
    }

    void releaseFirstCall()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            firstCallReleased_ = true;
        }
        changed_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    bool firstCallEntered_ = false;
    bool firstCallReleased_ = false;
};

class BlockingResolver final : public IEpgArtworkResolver
{
public:
    std::atomic<int> calls{0};

    EpgArtworkResolution resolve(
        const std::string&,
        const VdrEvent&) override
    {
        ++calls;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            entered_ = true;
        }
        changed_.notify_all();

        std::unique_lock<std::mutex> lock(mutex_);
        changed_.wait(lock, [this]() { return released_; });
        return {};
    }

    bool waitUntilEntered(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return changed_.wait_for(lock, timeout, [this]() { return entered_; });
    }

    void release()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            released_ = true;
        }
        changed_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    bool entered_ = false;
    bool released_ = false;
};

static VdrEvent event(const char* id)
{
    VdrEvent value;
    value.id = id;
    value.channelId = "S19.2E-1-1011-11100";
    value.title = id;
    return value;
}

int main()
{
    const char* path = "/tmp/vdr-suite-epg-artwork-enrichment-test.db";
    std::remove(path);

    Database database;
    assert(database.open(path));

    EpgArtworkRepository repository(database);
    FakeResolver resolver;
    EpgArtworkEnrichmentService service(
        repository,
        resolver,
        3,
        std::chrono::seconds(0),
        std::chrono::milliseconds(100),
        std::chrono::milliseconds(100));

    EpgArtworkReference stale;
    stale.backendId = "home";
    stale.channelId = "S19.2E-1-1011-11100";
    stale.eventId = "missing";
    stale.provider = "tvscraper";
    stale.path = "/cache/stale.jpg";
    stale.width = 1;
    stale.height = 1;
    stale.resolvedAt = 1;
    assert(repository.upsert(stale));

    const EpgArtworkEnrichmentResult scheduled = service.enrich(
        "home",
        {event("found"), event("missing"), event("unavailable")});
    assert(scheduled.queueAvailable);
    assert(scheduled.queued == 3);
    assert(scheduled.deduplicated == 0);
    assert(scheduled.suppressed == 0);
    assert(scheduled.dropped == 0);

    assert(resolver.waitUntilFirstCallEntered(std::chrono::seconds(5)));
    const EpgArtworkEnrichmentResult duplicate = service.enrich(
        "home",
        {event("found")});
    assert(duplicate.queued == 0);
    assert(duplicate.deduplicated == 1);
    resolver.releaseFirstCall();

    assert(service.waitUntilIdle(std::chrono::seconds(10)));
    assert(resolver.calls == 3);

    const EpgArtworkReference found = repository.find(
        "home", "S19.2E-1-1011-11100", "found");
    assert(found.valid());
    assert(found.path == "/cache/found.jpg");

    const EpgArtworkReference missing = repository.find(
        "home", "S19.2E-1-1011-11100", "missing");
    assert(!missing.valid());

    const EpgArtworkEnrichmentResult cooledDown = service.enrich(
        "home",
        {event("missing"), event("unavailable")});
    assert(cooledDown.queued == 0);
    assert(cooledDown.suppressed == 2);
    assert(resolver.calls == 3);

    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    const EpgArtworkEnrichmentResult retried = service.enrich(
        "home",
        {event("missing"), event("unavailable")});
    assert(retried.queued == 2);
    assert(retried.suppressed == 0);
    assert(service.waitUntilIdle(std::chrono::seconds(10)));
    assert(resolver.calls == 5);

    EpgArtworkEnrichmentService boundedService(
        repository,
        resolver,
        1,
        std::chrono::hours(24));
    const EpgArtworkEnrichmentResult bounded = boundedService.enrich(
        "home",
        {event("one"), event("two")});
    assert(bounded.queued == 1);
    assert(bounded.dropped == 1);
    assert(boundedService.waitUntilIdle(std::chrono::seconds(10)));

    BlockingResolver blockingResolver;
    auto shutdownService = std::make_unique<EpgArtworkEnrichmentService>(
        repository,
        blockingResolver,
        3,
        std::chrono::seconds(0));
    const EpgArtworkEnrichmentResult shutdownQueued = shutdownService->enrich(
        "home",
        {event("shutdown-one"), event("shutdown-two"), event("shutdown-three")});
    assert(shutdownQueued.queued == 3);
    assert(blockingResolver.waitUntilEntered(std::chrono::seconds(5)));

    std::thread shutdown([&shutdownService]() {
        shutdownService.reset();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    assert(blockingResolver.calls == 1);
    blockingResolver.release();
    shutdown.join();
    assert(blockingResolver.calls == 1);

    return 0;
}
