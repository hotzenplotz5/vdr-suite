#include "EpgCacheController.h"
#include "EpgCacheServiceRegistry.h"
#include "IEpgScraperMetadataResolver.h"

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
    writeFile(preferred, "preferred-image");
    writeFile(person, "person-image");
    writeFile(gallery, "gallery-image");

    EpgCacheServiceRegistry cacheRegistry;
    EpgCacheController controller(cacheRegistry);
    controller.setScraperMetadataAllowedRoots({root.string()});

    {
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 503);
        assert(contains(response.body, "backend unavailable"));
    }

    FakeResolver resolver;
    resolver.resolution = readyResolution(
        preferred.string(),
        person.string(),
        gallery.string());
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
        assert(resolver.lastBackendId == "default");
        assert(resolver.lastChannelId == "channel-1");
        assert(resolver.lastEventId == "event-1");
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
        resolver.resolution = readyResolution(
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
        resolver.resolution = EpgScraperMetadataResolution{};
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 502);
        assert(contains(response.body, "lookup failed"));
    }

    {
        resolver.resolution.attempted = true;
        resolver.resolution.found = false;
        const ApiResponse response = controller.getMetadata(
            "default",
            "channel-1",
            "event-1");
        assert(response.statusCode == 200);
        assert(contains(response.body, "\"status\":\"not-found\""));
    }

    std::filesystem::remove_all(root);
    return 0;
}
