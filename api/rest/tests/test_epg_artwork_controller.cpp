#include "Database.h"
#include "EpgArtworkController.h"
#include "EpgArtworkReference.h"
#include "EpgArtworkRepository.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
std::filesystem::path makeTestRoot()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-test-epg-artwork-controller";

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root / "allowed", error);
    assert(!error);
    return root;
}

void writeBinaryFile(
    const std::filesystem::path& path,
    const std::string& content)
{
    std::ofstream file(path, std::ios::binary);
    assert(file.good());
    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    assert(file.good());
}

EpgArtworkReference makeArtwork(
    const std::string& eventId,
    const std::filesystem::path& path)
{
    EpgArtworkReference artwork;
    artwork.backendId = "default";
    artwork.channelId = "S19.2E-1-1019-10301";
    artwork.eventId = eventId;
    artwork.provider = "tvscraper";
    artwork.path = path.string();
    artwork.width = 640;
    artwork.height = 360;
    artwork.resolvedAt = 123456;
    return artwork;
}
}

int main()
{
    const std::filesystem::path root = makeTestRoot();
    const std::filesystem::path databasePath = root / "artwork.sqlite";
    const std::filesystem::path allowedRoot = root / "allowed";
    const std::filesystem::path jpegPath = allowedRoot / "still.jpg";
    const std::filesystem::path webpPath = allowedRoot / "poster.webp";
    const std::filesystem::path textPath = allowedRoot / "still.txt";
    const std::filesystem::path outsidePath = root / "outside.jpg";

    const std::string jpegBytes("\xff\xd8\xff\xe0test-jpeg", 13);
    const std::string webpBytes("RIFF\x0c\x00\x00\x00WEBPVP8 ", 16);
    writeBinaryFile(jpegPath, jpegBytes);
    writeBinaryFile(webpPath, webpBytes);
    writeBinaryFile(textPath, "not-an-image");
    writeBinaryFile(outsidePath, jpegBytes);

    Database database;
    assert(database.open(databasePath.string()));

    EpgArtworkRepository repository(database);
    assert(repository.ensureSchema());
    assert(repository.upsert(makeArtwork("1001", jpegPath)));
    assert(repository.upsert(makeArtwork("1002", outsidePath)));
    assert(repository.upsert(makeArtwork("1003", textPath)));
    assert(repository.upsert(makeArtwork("1004", webpPath)));

    EpgArtworkController controller(
        repository,
        std::vector<std::string>{allowedRoot.string()});

    const ApiResponse found = controller.getArtwork(
        "default",
        "S19.2E-1-1019-10301",
        "1001");
    assert(found.statusCode == 200);
    assert(found.contentType == "image/jpeg");
    assert(found.body == jpegBytes);
    assert(found.body.find(jpegPath.string()) == std::string::npos);

    const ApiResponse webp = controller.getArtwork(
        "default",
        "S19.2E-1-1019-10301",
        "1004");
    assert(webp.statusCode == 200);
    assert(webp.contentType == "image/webp");
    assert(webp.body == webpBytes);
    assert(webp.body.find(webpPath.string()) == std::string::npos);

    const ApiResponse missing = controller.getArtwork(
        "default",
        "S19.2E-1-1019-10301",
        "9999");
    assert(missing.statusCode == 404);
    assert(missing.contentType == "application/json");

    const ApiResponse forbidden = controller.getArtwork(
        "default",
        "S19.2E-1-1019-10301",
        "1002");
    assert(forbidden.statusCode == 403);
    assert(forbidden.body.find(outsidePath.string()) == std::string::npos);

    const ApiResponse unsupported = controller.getArtwork(
        "default",
        "S19.2E-1-1019-10301",
        "1003");
    assert(unsupported.statusCode == 415);

    const ApiResponse invalid = controller.getArtwork(
        "default",
        "",
        "1001");
    assert(invalid.statusCode == 400);

    database.close();

    std::error_code error;
    std::filesystem::remove_all(root, error);
    return 0;
}
