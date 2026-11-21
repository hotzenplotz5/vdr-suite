#include "Database.h"
#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheController.h"
#include "EpgCacheServiceRegistry.h"
#include "IEpgScraperMetadataResolver.h"
#include "IEpgSeriesArtworkFallbackDeliveryProvider.h"
#include "PersistentEpgScraperMetadataResolver.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
bool contains(const std::string& text, const std::string& value)
{
    return text.find(value) != std::string::npos;
}

EpgScraperArtwork artwork(
    const std::string& path,
    int width,
    int height)
{
    EpgScraperArtwork value;
    value.available = true;
    value.provider = "tvscraper";
    value.path = path;
    value.width = width;
    value.height = height;
    return value;
}

class FakeResolver final
    : public IEpgScraperMetadataResolver,
      public IEpgSeriesArtworkFallbackDeliveryProvider
{
public:
    EpgScraperMetadataResolution resolve(
        const std::string& backendId,
        const VdrEvent& event) override
    {
        ++calls;
        lastBackendId = backendId;
        lastChannelId = event.channelId;
        lastEventId = event.id;

        EpgScraperMetadataResolution result = resolution;
        if (result.found)
        {
            result.metadata.backendId = backendId;
            result.metadata.channelId = event.channelId;
            result.metadata.eventId = event.id;
        }
        return result;
    }

    EpgSeriesArtworkFallbackAsset loadSeriesArtworkFallback(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override
    {
        ++fallbackCalls;
        fallbackBackendId = backendId;
        fallbackChannelId = channelId;
        fallbackEventId = eventId;
        return fallbackAsset;
    }

    EpgScraperMetadataResolution resolution;
    EpgSeriesArtworkFallbackAsset fallbackAsset;
    int calls = 0;
    mutable int fallbackCalls = 0;
    std::string lastBackendId;
    std::string lastChannelId;
    std::string lastEventId;
    mutable std::string fallbackBackendId;
    mutable std::string fallbackChannelId;
    mutable std::string fallbackEventId;
};

EpgScraperMetadataResolution readyResolution(
    const std::string& preferredPath,
    const std::string& personPath,
    const std::string& galleryPath)
{
    EpgScraperMetadataResolution resolution;
    resolution.attempted = true;
    resolution.found = true;

    EpgScraperMetadata& metadata = resolution.metadata;
    metadata.provider = "tvscraper";
    metadata.mediaType = EpgScraperMediaType::Series;
    metadata.providerId = 88;
    metadata.title = "Bares für Rares";
    metadata.originalTitle = "Bares für Rares";
    metadata.episodeName = "Die Trödel-Show";
    metadata.overview = "Ausführliche TVScraper-Beschreibung";
    metadata.imdbId = "tt1234567";
    metadata.genres = {"Show"};
    metadata.preferredArtwork = artwork(preferredPath, 1280, 720);

    EpgScraperPerson person;
    person.role = EpgScraperPersonRole::Moderator;
    person.name = "Horst Lichter";
    person.characterName = "Moderator";
    person.image = artwork(personPath, 300, 450);
    metadata.people.push_back(person);

    EpgScraperImage image;
    image.orientation = EpgScraperImageOrientation::Portrait;
    image.artwork = artwork(galleryPath, 600, 900);
    metadata.images.push_back(image);
    return resolution;
}

void writeFile(const std::filesystem::path& path, const std::string& data)
{
    std::ofstream file(path, std::ios::binary);
    assert(file.good());
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    assert(file.good());
}

VdrEvent event(const std::string& eventId)
{
    VdrEvent value;
    value.channelId = "channel-1";
    value.id = eventId;
    value.title = "Bares für Rares";
    return value;
}
}

int main()
{
    const std::filesystem::path root =
        "/tmp/vdr-suite-epg-scraper-metadata-controller";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::filesystem::path preferred = root / "preferred.jpg";
    const std::filesystem::path person = root / "person.png";
    const std::filesystem::path gallery = root / "gallery.jpg";
    const std::filesystem::path databasePath = root / "metadata.db";
    writeFile(preferred, "preferred-image");
    writeFile(person, "person-image");
    writeFile(gallery, "gallery-image");

    Database database;
    assert(database.open(databasePath.string()));
    EpgArtworkRepository artworkRepository(database);
    assert(artworkRepository.ensureSchema());
    EpgArtworkPublicJsonSerializer artworkJsonSerializer;
    EpgCacheServiceRegistry cacheRegistry;

    FakeResolver delegate;
    delegate.resolution = readyResolution(
        preferred.string(),
        person.string(),
        gallery.string());
    PersistentEpgScraperMetadataResolver resolver(
        delegate,
        artworkRepository,
        {root.string()});

    EpgCacheController controller(
        cacheRegistry,
        artworkRepository,
        artworkJsonSerializer);
    controller.setScraperMetadataAllowedRoots({root.string()});
    controller.registerScraperMetadataResolver("default", resolver);

    const ApiResponse pending = controller.getMetadata(
        "default",
        "channel-1",
        "event-1");
    assert(pending.statusCode == 200);
    assert(contains(pending.body, "\"status\":\"pending\""));
    assert(delegate.calls == 0);

    const EpgScraperMetadataResolution materialized = resolver.resolve(
        "default",
        event("event-1"));
    assert(materialized.attempted);
    assert(materialized.found);
    assert(delegate.calls == 1);

    const ApiResponse first = controller.getMetadata(
        "",
        "channel-1",
        "event-1");
    assert(first.statusCode == 200);
    assert(first.contentType == "application/json");
    assert(contains(first.body, "\"available\":true"));
    assert(contains(first.body, "Bares für Rares"));
    assert(contains(first.body, "Horst Lichter"));
    assert(contains(first.body, "kind=person&index=0"));
    assert(!contains(first.body, root.string()));
    assert(delegate.calls == 1);

    const EpgArtworkReference persisted = artworkRepository.find(
        "default",
        "channel-1",
        "event-1");
    assert(persisted.valid());
    assert(persisted.path == preferred.string());
    assert(!artworkRepository.findMetadataJson(
        "default", "channel-1", "event-1").empty());
    assert(artworkRepository.findMetadataImage(
        "default", "channel-1", "event-1", "person", 0).valid());
    assert(artworkRepository.findMetadataImage(
        "default", "channel-1", "event-1", "gallery", 0).valid());

    delegate.resolution = EpgScraperMetadataResolution{};

    const ApiResponse cachedMetadata = controller.getMetadata(
        "default",
        "channel-1",
        "event-1");
    assert(cachedMetadata.statusCode == 200);
    assert(cachedMetadata.body == first.body);
    assert(delegate.calls == 1);

    delegate.fallbackAsset.contentType = "image/png";
    delegate.fallbackAsset.content = "fallback-image";
    delegate.fallbackAsset.width = 1920;
    delegate.fallbackAsset.height = 1080;

    const ApiResponse preferredImage = controller.getMetadataImage(
        "default", "channel-1", "event-1", "preferred", 0);
    assert(preferredImage.statusCode == 200);
    assert(preferredImage.contentType == "image/jpeg");
    assert(preferredImage.body == "preferred-image");
    assert(delegate.calls == 1);
    assert(delegate.fallbackCalls == 0);

    const ApiResponse personImage = controller.getMetadataImage(
        "default", "channel-1", "event-1", "person", 0);
    assert(personImage.statusCode == 200);
    assert(personImage.contentType == "image/png");
    assert(personImage.body == "person-image");
    assert(delegate.calls == 1);

    const ApiResponse galleryImage = controller.getMetadataImage(
        "default", "channel-1", "event-1", "gallery", 0);
    assert(galleryImage.statusCode == 200);
    assert(galleryImage.contentType == "image/jpeg");
    assert(galleryImage.body == "gallery-image");
    assert(delegate.calls == 1);

    assert(controller.getMetadataImage(
        "default", "channel-1", "event-1", "unsupported", 0).statusCode == 400);

    const ApiResponse fallbackImage = controller.getMetadataImage(
        "default", "channel-1", "event-fallback", "preferred", 0);
    assert(fallbackImage.statusCode == 200);
    assert(fallbackImage.contentType == "image/png");
    assert(fallbackImage.body == "fallback-image");
    assert(delegate.fallbackCalls == 1);
    assert(delegate.fallbackBackendId == "default");
    assert(delegate.fallbackChannelId == "channel-1");
    assert(delegate.fallbackEventId == "event-fallback");

    const int callsBeforeNonPreferred = delegate.fallbackCalls;
    assert(controller.getMetadataImage(
        "default", "channel-1", "event-fallback", "person", 0).statusCode == 404);
    assert(delegate.fallbackCalls == callsBeforeNonPreferred);

    delegate.fallbackAsset.contentType = "text/plain";
    assert(controller.getMetadataImage(
        "default", "channel-1", "event-invalid", "preferred", 0).statusCode == 404);
    assert(delegate.fallbackCalls == callsBeforeNonPreferred + 1);

    EpgCacheController restartedController(
        cacheRegistry,
        artworkRepository,
        artworkJsonSerializer);
    restartedController.setScraperMetadataAllowedRoots({root.string()});
    restartedController.registerScraperMetadataResolver("default", resolver);

    const ApiResponse afterRestart = restartedController.getMetadata(
        "default", "channel-1", "event-1");
    assert(afterRestart.statusCode == 200);
    assert(afterRestart.body == first.body);
    assert(delegate.calls == 1);

    const ApiResponse imageAfterRestart = restartedController.getMetadataImage(
        "default", "channel-1", "event-1", "preferred", 0);
    assert(imageAfterRestart.statusCode == 200);
    assert(imageAfterRestart.body == "preferred-image");
    assert(delegate.calls == 1);

    const ApiResponse unknownEvent = restartedController.getMetadata(
        "default", "channel-1", "event-2");
    assert(unknownEvent.statusCode == 200);
    assert(contains(unknownEvent.body, "\"status\":\"pending\""));
    assert(delegate.calls == 1);
    delegate.fallbackAsset = EpgSeriesArtworkFallbackAsset{};
    assert(restartedController.getMetadataImage(
        "default", "channel-1", "event-2", "preferred", 0).statusCode == 404);

    delegate.resolution = readyResolution(
        "/etc/passwd.jpg",
        person.string(),
        gallery.string());
    const EpgScraperMetadataResolution rejectedReplacement = resolver.resolve(
        "default",
        event("event-1"));
    assert(rejectedReplacement.found);
    assert(rejectedReplacement.metadata.preferredArtwork.valid());
    assert(rejectedReplacement.metadata.preferredArtwork.path == preferred.string());
    assert(delegate.calls == 2);

    const EpgArtworkReference retained = artworkRepository.find(
        "default",
        "channel-1",
        "event-1");
    assert(retained.valid());
    assert(retained.path == preferred.string());

    const ApiResponse retainedMetadata = restartedController.getMetadata(
        "default", "channel-1", "event-1");
    assert(retainedMetadata.statusCode == 200);
    assert(contains(retainedMetadata.body, "\"preferredArtwork\":{\"available\":true"));
    assert(delegate.calls == 2);

    delegate.resolution = readyResolution(
        "/etc/passwd.jpg",
        "/etc/passwd.png",
        "/etc/passwd.jpg");
    const EpgScraperMetadataResolution unsafeOnly = resolver.resolve(
        "default",
        event("event-3"));
    assert(unsafeOnly.found);
    assert(!unsafeOnly.metadata.preferredArtwork.valid());
    assert(!unsafeOnly.metadata.people.front().image.valid());
    assert(!unsafeOnly.metadata.images.front().artwork.valid());
    assert(restartedController.getMetadataImage(
        "default", "channel-1", "event-3", "preferred", 0).statusCode == 404);

    database.close();
    std::filesystem::remove_all(root);
    return 0;
}
