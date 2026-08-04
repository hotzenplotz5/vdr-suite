#include "Database.h"
#include "EpgSeriesArtworkFallbackRepository.h"
#include "PersistentSeriesArtworkFallbackResolver.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{
class Delegate final : public IEpgScraperMetadataResolver
{
public:
    EpgScraperMetadataResolution value;
    EpgScraperMetadataResolution resolve(
        const std::string&,
        const VdrEvent&) override
    {
        return value;
    }
};

EpgScraperMetadataResolution resolutionFor(
    const std::filesystem::path& path)
{
    EpgScraperMetadataResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.metadata.backendId = "backend";
    resolution.metadata.channelId = "channel";
    resolution.metadata.eventId = "event";
    resolution.metadata.provider = "tvscraper";
    resolution.metadata.mediaType = EpgScraperMediaType::Series;
    resolution.metadata.seriesArtworkFallback.available = true;
    resolution.metadata.seriesArtworkFallback.provider = "provider";
    resolution.metadata.seriesArtworkFallback.origin =
        EpgScraperArtworkOrigin::ExternalFallback;
    resolution.metadata.seriesArtworkFallback.managed = true;
    resolution.metadata.seriesArtworkFallback.path = path.string();
    resolution.metadata.seriesArtworkFallback.width = 640;
    resolution.metadata.seriesArtworkFallback.height = 360;
    return resolution;
}
}

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-persistent-fallback-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "cache");
    const std::filesystem::path cached = root / "cache" / "series.png";
    std::ofstream(cached, std::ios::binary) << "png";

    Database database;
    assert(database.open(":memory:"));
    EpgSeriesArtworkFallbackRepository repository(database);
    assert(repository.ensureSchema());

    VdrEvent event;
    event.channelId = "channel";
    event.id = "event";

    Delegate delegate;
    delegate.value = resolutionFor(cached);
    PersistentSeriesArtworkFallbackResolver resolver(
        delegate,
        repository,
        {(root / "cache").string()});

    auto resolution = resolver.resolve("backend", event);
    assert(resolution.metadata.seriesArtworkFallback.available);
    assert(resolution.metadata.seriesArtworkFallback.managed);
    const EpgArtworkReference persisted = repository.find(
        "backend",
        "channel",
        "event");
    assert(persisted.valid());
    assert(persisted.origin == EpgArtworkReferenceOrigin::ExternalFallback);

    delegate.value = resolutionFor(cached);
    delegate.value.metadata.seriesArtworkFallback = EpgScraperArtwork{};
    resolution = resolver.resolve("backend", event);
    assert(resolution.metadata.seriesArtworkFallback.available);
    assert(resolution.metadata.seriesArtworkFallback.provider == "provider");
    assert(resolution.metadata.seriesArtworkFallback.origin ==
           EpgScraperArtworkOrigin::ExternalFallback);
    assert(resolution.metadata.seriesArtworkFallback.managed);
    assert(resolution.metadata.seriesArtworkFallback.path == cached.string());

    const std::filesystem::path outside = root / "outside.png";
    std::ofstream(outside, std::ios::binary) << "outside";
    delegate.value = resolutionFor(outside);
    assert(repository.removeForEvent("backend", "channel", "event"));
    resolution = resolver.resolve("backend", event);
    assert(!resolution.metadata.seriesArtworkFallback.available);
    assert(!resolution.metadata.seriesArtworkFallback.managed);

    delegate.value = resolutionFor(cached);
    delegate.value.metadata.seriesArtworkFallback.provider = "tvscraper";
    resolution = resolver.resolve("backend", event);
    assert(!resolution.metadata.seriesArtworkFallback.available);

    delegate.value = resolutionFor(cached);
    delegate.value.metadata.seriesArtworkFallback.origin =
        EpgScraperArtworkOrigin::Unknown;
    resolution = resolver.resolve("backend", event);
    assert(!resolution.metadata.seriesArtworkFallback.available);

    delegate.value = resolutionFor(cached);
    delegate.value.found = false;
    resolution = resolver.resolve("backend", event);
    assert(!resolution.metadata.seriesArtworkFallback.available);

    std::filesystem::remove_all(root);
    return 0;
}
