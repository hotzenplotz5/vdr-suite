#include "Database.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingArtworkService.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

const std::filesystem::path testRoot =
    "/tmp/vdr-suite-artwork-service-root";
const std::filesystem::path outsideRoot =
    "/tmp/vdr-suite-artwork-service-outside";
const char* databasePath =
    "/tmp/test_vdr_recording_artwork_service.db";

void writeBinary(
    const std::filesystem::path& path,
    const std::string& content)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    assert(file.good());
    file.write(
        content.data(),
        static_cast<std::streamsize>(content.size()));
    assert(file.good());
}

VdrRecordingArtworkRef makeArtwork(
    const VdrRecordingArtworkKind kind,
    const std::string& reference)
{
    VdrRecordingArtworkRef artwork;
    artwork.kind = kind;
    artwork.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    artwork.reference = reference;
    artwork.width = 680;
    artwork.height = 1000;
    return artwork;
}

VdrRecording makeRecording()
{
    VdrRecording recording;
    recording.id = "7";
    recording.backendId = "default";
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Forrest_Gump/recording.rec";
    recording.path =
        "/Movies/Forrest_Gump/recording.rec";
    recording.title = "Forrest Gump";
    recording.metadata.provider.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    recording.metadata.provider.contentKind =
        VdrRecordingContentKind::Movie;
    recording.metadata.provider.movieId = "13";
    recording.metadata.provider.title = "Forrest Gump";
    recording.metadata.artwork.push_back(makeArtwork(
        VdrRecordingArtworkKind::Poster,
        "movies/13/poster.jpg"));
    recording.metadata.artwork.push_back(makeArtwork(
        VdrRecordingArtworkKind::Banner,
        "movies/13/notes.txt"));
    recording.metadata.artwork.push_back(makeArtwork(
        VdrRecordingArtworkKind::Fanart,
        "movies/13/escape.jpg"));
    return recording;
}

}

int main()
{
    std::error_code error;
    std::filesystem::remove_all(testRoot, error);
    std::filesystem::remove_all(outsideRoot, error);
    std::remove(databasePath);

    const std::string jpegBytes(
        "\xff\xd8VDR-SUITE\xff\xd9",
        13);
    writeBinary(testRoot / "movies/13/poster.jpg", jpegBytes);
    writeBinary(testRoot / "movies/13/notes.txt", "not an image");
    writeBinary(outsideRoot / "outside.jpg", "outside");

    std::filesystem::create_directories(testRoot / "movies/13");
    std::filesystem::create_symlink(
        outsideRoot / "outside.jpg",
        testRoot / "movies/13/escape.jpg",
        error);
    assert(!error);

    Database database;
    assert(database.open(databasePath));

    VdrRecordingCacheRepository repository(database);
    assert(repository.ensureSchema());

    const VdrRecording recording = makeRecording();
    assert(repository.replaceRecordingsForBackend(
        "default",
        {recording}));

    const VdrRecordingArtworkRef& poster =
        recording.metadata.artwork.at(0);
    const VdrRecordingArtworkRef& unsupported =
        recording.metadata.artwork.at(1);
    const VdrRecordingArtworkRef& escaped =
        recording.metadata.artwork.at(2);

    const std::string posterId =
        VdrRecordingArtworkIdentity::assetId(
            recording,
            poster);
    const std::string posterUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            recording,
            poster);

    assert(VdrRecordingArtworkIdentity::isValidAssetId(posterId));
    assert(posterId.size() == 32);
    assert(posterUrl ==
           "/recording-artwork/default/" + posterId);
    assert(posterUrl.find("movies") == std::string::npos);
    assert(posterUrl.find("poster.jpg") == std::string::npos);
    assert(VdrRecordingArtworkIdentity::preferredArtwork(recording) ==
           &recording.metadata.artwork.at(0));

    VdrRecordingArtworkService service(
        repository,
        {testRoot.string()});

    assert(service.handlesPath(posterUrl));
    assert(!service.handlesPath("/api/recordings"));

    const VdrRecordingArtworkAsset asset =
        service.loadPath(posterUrl);
    assert(asset.found());
    assert(asset.statusCode == 200);
    assert(asset.contentType == "image/jpeg");
    assert(asset.content == jpegBytes);

    assert(!service.loadPath(
        "/recording-artwork/other/" + posterId).found());
    assert(!service.loadPath(
        "/recording-artwork/default/not-an-asset-id").found());
    assert(!service.loadPath(
        "/recording-artwork/default/" +
        posterId.substr(0, 31) + "g").found());
    assert(!service.loadPath(
        "/recording-artwork/default%2Fescape/" + posterId).found());

    const std::string unsupportedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            recording,
            unsupported);
    assert(!service.loadPath(unsupportedUrl).found());

    const std::string escapedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            recording,
            escaped);
    assert(!service.loadPath(escapedUrl).found());

    VdrRecordingArtworkService sizeLimitedService(
        repository,
        {testRoot.string()},
        4);
    assert(!sizeLimitedService.loadPath(posterUrl).found());

    std::filesystem::remove_all(testRoot, error);
    std::filesystem::remove_all(outsideRoot, error);
    std::remove(databasePath);

    std::cout
        << "test_vdr_recording_artwork_service passed"
        << std::endl;
    return 0;
}
