#include "Database.h"
#include "EpgArtworkEnrichmentService.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

class FakeResolver final : public IEpgArtworkResolver
{
public:
    std::atomic<int> calls{0};

    EpgArtworkResolution resolve(
        const std::string&,
        const VdrEvent& event) override
    {
        ++calls;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

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
        std::chrono::seconds(0));

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
    assert(scheduled.dropped == 0);

    const EpgArtworkEnrichmentResult duplicate = service.enrich(
        "home",
        {event("found")});
    assert(duplicate.queued == 0);
    assert(duplicate.deduplicated == 1);

    assert(service.waitUntilIdle(std::chrono::seconds(2)));
    assert(resolver.calls == 3);

    const EpgArtworkReference found = repository.find(
        "home", "S19.2E-1-1011-11100", "found");
    assert(found.valid());
    assert(found.path == "/cache/found.jpg");

    const EpgArtworkReference missing = repository.find(
        "home", "S19.2E-1-1011-11100", "missing");
    assert(!missing.valid());

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
    assert(boundedService.waitUntilIdle(std::chrono::seconds(2)));

    return 0;
}
