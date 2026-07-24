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
    EpgCacheController controller(
        cacheRegistry,
        artworkRepository,
        artworkJsonSerializer);
    controller.setScraperMetadataAllowedRoots({root.string()});

    {
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 503);
        assert(contains(response.body, "backend unavailable"));
    }

    FakeResolver delegate;
    delegate.resolution = readyResolution(
        preferred.string(),
        person.string(),
        gallery.string());
    PersistentEpgScraperMetadataResolver resolver(
        delegate,
        artworkRepository);
    controller.registerScraperMetadataResolver("default", resolver);

    {
        const ApiResponse response = controller.getMetadata(
            "",
            "channel-1",
            "event-1");
        assert(response.statusCode == 200);
        assert(response.contentType == "application/json");
        assert(contains(response.body, "\"available\":true"));
        assert(contains(response.body, "Bares für Rares"));
        assert(contains(response.body, "Horst Lichter"));
        assert(contains(response.body, "kind=person&index=0"));
        assert(!contains(response.body, root.string()));
        assert(!contains(response.body, "preferred.jpg"));
        assert(delegate.lastBackendId == "default");
        assert(delegate.lastChannelId == "channel-1");
        assert(delegate.lastEventId == "event-1");

        const EpgArtworkReference persisted = artworkRepository.find(
            "default",
            "channel-1",
            "event-1");
        assert(persisted.valid());
        assert(persisted.path == preferred.string());
        assert(persisted.width == 1280);
        assert(persisted.height == 720);
        assert(persisted.resolvedAt > 0);
    }

    {
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "preferred",
            0);
        assert(response.statusCode == 200);
        assert(response.contentType == "image/jpeg");
        assert(response.body == "preferred-image");
    }

    {
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "person",
            0);
        assert(response.statusCode == 200);
        assert(response.contentType == "image/png");
        assert(response.body == "person-image");
    }

    {
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "gallery",
            0);
        assert(response.statusCode == 200);
        assert(response.contentType == "image/jpeg");
        assert(response.body == "gallery-image");
    }

    {
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "unsupported",
            0);
        assert(response.statusCode == 400);
    }

    {
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "gallery",
            7);
        assert(response.statusCode == 404);
    }

    {
        delegate.resolution = readyResolution(
            "/etc/passwd.jpg",
            person.string(),
            gallery.string());
        const ApiResponse response = controller.getMetadataImage(
            "default",
            "channel-1",
            "event-1",
            "preferred",
            0);
        assert(response.statusCode == 403);
    }

    {
        delegate.resolution = EpgScraperMetadataResolution{};
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 502);
        assert(contains(response.body, "lookup failed"));

        const EpgArtworkReference retained = artworkRepository.find(
            "default",
            "channel-1",
            "event-1");
        assert(retained.valid());
        assert(retained.path == preferred.string());
    }

    {
        delegate.resolution.attempted = true;
        delegate.resolution.found = false;
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 200);
        assert(contains(response.body, "\"status\":\"not-found\""));

        const EpgArtworkReference retained = artworkRepository.find(
            "default",
            "channel-1",
            "event-1");
        assert(retained.valid());
        assert(retained.path == preferred.string());
    }

    database.close();
    std::filesystem::remove_all(root);
    return 0;
}
