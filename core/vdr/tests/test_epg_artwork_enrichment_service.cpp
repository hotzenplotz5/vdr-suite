#include "Database.h"
#include "EpgArtworkEnrichmentService.h"

#include <cassert>
#include <cstdio>

class FakeResolver final : public IEpgArtworkResolver
{
public:
    EpgArtworkResolution resolve(
        const std::string&,
        const VdrEvent& event) override
    {
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
        result.artwork.resolvedAt = "1234";
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
    EpgArtworkEnrichmentService service(repository, resolver);

    EpgArtworkReference stale;
    stale.backendId = "home";
    stale.channelId = "S19.2E-1-1011-11100";
    stale.eventId = "missing";
    stale.provider = "tvscraper";
    stale.path = "/cache/stale.jpg";
    stale.width = 1;
    stale.height = 1;
    stale.resolvedAt = "1";
    assert(repository.upsert(stale));

    const EpgArtworkEnrichmentResult result = service.enrich(
        "home",
        {event("found"), event("missing"), event("unavailable")});

    assert(result.attempted == 2);
    assert(result.stored == 1);
    assert(result.removed == 1);
    assert(result.unavailable == 1);
    assert(result.repositoryOk);

    const EpgArtworkReference found = repository.find(
        "home", "S19.2E-1-1011-11100", "found");
    assert(found.valid());
    assert(found.path == "/cache/found.jpg");

    const EpgArtworkReference missing = repository.find(
        "home", "S19.2E-1-1011-11100", "missing");
    assert(!missing.valid());

    return 0;
}
