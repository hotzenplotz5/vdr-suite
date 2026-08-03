#include "Database.h"
#include "EpgArtworkRepository.h"
#include "IEpgScraperMetadataResolver.h"
#include "PersistentEpgScraperMetadataResolver.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
class FakeResolver final : public IEpgScraperMetadataResolver
{
public:
    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override
    {
        EpgScraperMetadataResolution result = resolution;
        result.metadata.backendId = backendId.empty() ? "default" : backendId;
        result.metadata.channelId = event.channelId;
        result.metadata.eventId = event.id;
        return result;
    }

    EpgScraperMetadataResolution resolution;
};

void writeFile(const std::filesystem::path& path)
{
    std::ofstream file(path, std::ios::binary);
    assert(file.good());
    file << "image-bytes";
    assert(file.good());
}

EpgScraperMetadataResolution baseResolution()
{
    EpgScraperMetadataResolution resolution;
    resolution.attempted = true;
    resolution.found = true;
    resolution.metadata.provider = "tvscraper";
    resolution.metadata.mediaType = EpgScraperMediaType::Series;
    resolution.metadata.providerId = 17;
    resolution.metadata.title = "Testserie";
    return resolution;
}

EpgScraperArtwork fallbackArtwork(const std::filesystem::path& path)
{
    EpgScraperArtwork artwork;
    artwork.available = true;
    artwork.provider = "tvmaze";
    artwork.origin = EpgScraperArtworkOrigin::ExternalFallback;
    artwork.managed = true;
    artwork.path = path.string();
    artwork.width = 680;
    artwork.height = 1000;
    return artwork;
}

EpgScraperArtwork primaryArtwork(const std::filesystem::path& path)
{
    EpgScraperArtwork artwork;
    artwork.available = true;
    artwork.provider = "tvscraper";
    artwork.origin = EpgScraperArtworkOrigin::PrimaryMetadata;
    artwork.path = path.string();
    artwork.width = 1280;
    artwork.height = 720;
    return artwork;
}

VdrEvent event(const std::string& eventId)
{
    VdrEvent value;
    value.channelId = "channel-1";
    value.id = eventId;
    value.title = "Testserie";
    return value;
}
}

int main()
{
    const std::filesystem::path root =
        "/tmp/vdr-suite-persistent-epg-fallback-artwork";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::filesystem::path fallbackPath = root / "fallback.jpg";
    const std::filesystem::path primaryPath = root / "primary.jpg";
    const std::filesystem::path databasePath = root / "metadata.db";
    writeFile(fallbackPath);
    writeFile(primaryPath);

    Database database;
    assert(database.open(databasePath.string()));

    EpgArtworkRepository artworkRepository(database);
    assert(artworkRepository.ensureSchema());

    FakeResolver delegate;
    PersistentEpgScraperMetadataResolver resolver(
        delegate,
        artworkRepository,
        {root.string()});

    delegate.resolution = baseResolution();
    delegate.resolution.metadata.seriesArtworkFallback =
        fallbackArtwork(fallbackPath);

    const EpgScraperMetadataResolution fallbackResult = resolver.resolve(
        "default",
        event("event-fallback"));
    assert(fallbackResult.found);
    assert(!fallbackResult.metadata.preferredArtwork.valid());
    assert(fallbackResult.metadata.seriesArtworkFallback.managed);

    const EpgArtworkReference overviewFallback = artworkRepository.find(
        "default",
        "channel-1",
        "event-fallback");
    assert(overviewFallback.valid());
    assert(overviewFallback.provider == "tvmaze");
    assert(overviewFallback.path == fallbackPath.string());
    assert(overviewFallback.width == 680);
    assert(overviewFallback.height == 1000);

    const std::string fallbackJson = artworkRepository.findMetadataJson(
        "default",
        "channel-1",
        "event-fallback");
    assert(fallbackJson.find(
        "\"preferredArtwork\":{\"available\":true") !=
        std::string::npos);

    delegate.resolution = baseResolution();
    delegate.resolution.metadata.preferredArtwork = primaryArtwork(primaryPath);
    delegate.resolution.metadata.seriesArtworkFallback =
        fallbackArtwork(fallbackPath);

    const EpgScraperMetadataResolution primaryResult = resolver.resolve(
        "default",
        event("event-primary"));
    assert(primaryResult.found);
    assert(primaryResult.metadata.preferredArtwork.valid());

    const EpgArtworkReference overviewPrimary = artworkRepository.find(
        "default",
        "channel-1",
        "event-primary");
    assert(overviewPrimary.valid());
    assert(overviewPrimary.provider == "tvscraper");
    assert(overviewPrimary.path == primaryPath.string());

    database.close();
    std::filesystem::remove_all(root);
    return 0;
}
