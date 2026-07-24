#include "Database.h"
#include "EpgArtworkPublicJsonSerializer.h"
#include "EpgArtworkRepository.h"
#include "EpgCacheController.h"
#include "EpgCacheServiceRegistry.h"
#include "IEpgScraperMetadataResolver.h"
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

class FakeResolver final : public IEpgScraperMetadataResolver
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

    EpgScraperMetadataResolution resolution;
    int calls = 0;
    std::string lastBackendId;
    std::string lastChannelId;
    std::string lastEventId;
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
        artworkRepository);

    EpgCacheController controller(
        cacheRegistry,
        artworkRepository,
        artworkJsonSerializer);
    controller.setScraperMetadataAllowedRoots({root.string()});
    controller.registerScraperMetadataResolver("default", resolver);

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

    const ApiResponse preferredImage = controller.getMetadataImage(
        "default", "channel-1", "event-1", "preferred", 0);
    assert(preferredImage.statusCode == 200);
    assert(preferredImage.contentType == "image/jpeg");
    assert(preferredImage.body == "preferred-image");
    assert(delegate.calls == 1);

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

    delegate.resolution.attempted = true;
    delegate.resolution.found = false;
    const ApiResponse unknownEvent = restartedController.getMetadata(
        "default", "channel-1", "event-2");
    assert(unknownEvent.statusCode == 200);
    assert(contains(unknownEvent.body, "\"status\":\"not-found\""));
    assert(delegate.calls == 2);

    delegate.resolution = readyResolution(
        "/etc/passwd.jpg",
        person.string(),
        gallery.string());
    const ApiResponse forbidden = restartedController.getMetadataImage(
        "default", "channel-1", "event-3", "preferred", 0);
    assert(forbidden.statusCode == 403);

    database.close();
    std::filesystem::remove_all(root);
    return 0;
}
