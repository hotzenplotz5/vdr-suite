#include "Database.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingArtworkService.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

namespace
{

const std::filesystem::path testRoot =
    "/tmp/vdr-suite-artwork-service-root";
const std::filesystem::path remoteRoot =
    "/tmp/vdr-suite-artwork-service-remote";
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

std::string jpegBytes(
    const char marker)
{
    const char bytes[] = {
        static_cast<char>(0xff),
        static_cast<char>(0xd8),
        static_cast<char>(0xff),
        static_cast<char>(0xe0),
        'V',
        'D',
        'R',
        '-',
        marker};
    return std::string(bytes, sizeof(bytes));
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

VdrRecording makeRecording(
    const std::string& backendId,
    const std::string& recordingId)
{
    VdrRecording recording;
    recording.id = recordingId;
    recording.backendId = backendId;
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Forrest_Gump/" + backendId + ".rec";
    recording.path =
        "/Movies/Forrest_Gump/" + backendId + ".rec";
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
    recording.metadata.artwork.push_back(makeArtwork(
        VdrRecordingArtworkKind::Still,
        "movies/13/fake.jpg"));
    return recording;
}

}

int main()
{
    std::error_code error;
    std::filesystem::remove_all(testRoot, error);
    std::filesystem::remove_all(remoteRoot, error);
    std::filesystem::remove_all(outsideRoot, error);
    std::remove(databasePath);

    const std::string localJpegBytes = jpegBytes('L');
    const std::string remoteJpegBytes = jpegBytes('R');
    const std::string rootPrefixedJpegBytes = jpegBytes('P');
    writeBinary(
        testRoot / "movies/13/poster.jpg",
        localJpegBytes);
    writeBinary(
        remoteRoot / "movies/13/poster.jpg",
        remoteJpegBytes);
    writeBinary(
        testRoot / "movies/150_poster.jpg",
        rootPrefixedJpegBytes);
    writeBinary(
        testRoot / "movies/13/notes.txt",
        "not an image");
    writeBinary(
        testRoot / "movies/13/fake.jpg",
        "<html>not an image</html>");
    writeBinary(
        outsideRoot / "outside.jpg",
        localJpegBytes);

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

    VdrRecording localRecording =
        makeRecording("default", "7");
    localRecording.metadata.artwork.push_back(
        makeArtwork(
            VdrRecordingArtworkKind::Poster,
            testRoot
                .lexically_normal()
                .relative_path()
                .generic_string() +
                "/movies/150_poster.jpg"));

    const VdrRecording remoteRecording =
        makeRecording("wohnhaus2", "8");
    assert(repository.replaceRecordingsForBackend(
        "default",
        {localRecording}));
    assert(repository.replaceRecordingsForBackend(
        "wohnhaus2",
        {remoteRecording}));

    const VdrRecordingArtworkRef& poster =
        localRecording.metadata.artwork.at(0);
    const VdrRecordingArtworkRef& unsupported =
        localRecording.metadata.artwork.at(1);
    const VdrRecordingArtworkRef& escaped =
        localRecording.metadata.artwork.at(2);
    const VdrRecordingArtworkRef& disguised =
        localRecording.metadata.artwork.at(3);
    const VdrRecordingArtworkRef& rootPrefixed =
        localRecording.metadata.artwork.at(4);

    const std::string posterId =
        VdrRecordingArtworkIdentity::assetId(
            localRecording,
            poster);
    const std::string posterUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            localRecording,
            poster);

    VdrRecording snapshotRecording = localRecording;
    snapshotRecording.backendId.clear();
    const VdrRecordingArtworkRef& snapshotPoster =
        snapshotRecording.metadata.artwork.at(0);
    const std::string snapshotPosterId =
        VdrRecordingArtworkIdentity::assetId(
            snapshotRecording,
            snapshotPoster);
    const std::string snapshotPosterUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            snapshotRecording,
            snapshotPoster);

    assert(VdrRecordingArtworkIdentity::isValidAssetId(posterId));
    assert(posterId.size() == 32);
    assert(posterUrl ==
           "/recording-artwork/default/" + posterId);
    assert(snapshotPosterId == posterId);
    assert(snapshotPosterUrl == posterUrl);
    assert(posterUrl.find("movies") == std::string::npos);
    assert(posterUrl.find("poster.jpg") == std::string::npos);
    assert(VdrRecordingArtworkIdentity::preferredArtwork(localRecording) ==
           &localRecording.metadata.artwork.at(0));

    VdrRecordingArtworkService localService(
        repository,
        {{"default", testRoot.string()}});

    assert(localService.handlesPath(posterUrl));
    assert(!localService.handlesPath("/api/recordings"));

    const VdrRecordingArtworkAsset asset =
        localService.loadPath(posterUrl);
    assert(asset.found());
    assert(asset.statusCode == 200);
    assert(asset.contentType == "image/jpeg");
    assert(asset.content == localJpegBytes);

    const VdrRecordingArtworkAsset snapshotAsset =
        localService.loadPath(snapshotPosterUrl);
    assert(snapshotAsset.found());
    assert(snapshotAsset.statusCode == 200);
    assert(snapshotAsset.contentType == "image/jpeg");
    assert(snapshotAsset.content == localJpegBytes);

    const std::string rootPrefixedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            localRecording,
            rootPrefixed);
    const VdrRecordingArtworkAsset rootPrefixedAsset =
        localService.loadPath(rootPrefixedUrl);
    assert(rootPrefixedAsset.found());
    assert(rootPrefixedAsset.statusCode == 200);
    assert(rootPrefixedAsset.contentType == "image/jpeg");
    assert(rootPrefixedAsset.content == rootPrefixedJpegBytes);

    const std::string remotePosterUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            remoteRecording,
            remoteRecording.metadata.artwork.at(0));
    assert(!localService.loadPath(remotePosterUrl).found());

    VdrRecordingArtworkService multiBackendService(
        repository,
        {
            {"default", testRoot.string()},
            {"wohnhaus2", remoteRoot.string()}
        });
    const VdrRecordingArtworkAsset remoteAsset =
        multiBackendService.loadPath(remotePosterUrl);
    assert(remoteAsset.found());
    assert(remoteAsset.content == remoteJpegBytes);

    assert(!localService.loadPath(
        "/recording-artwork/other/" + posterId).found());
    assert(!localService.loadPath(
        "/recording-artwork/default/not-an-asset-id").found());
    assert(!localService.loadPath(
        "/recording-artwork/default/" +
        posterId.substr(0, 31) + "g").found());
    assert(!localService.loadPath(
        "/recording-artwork/default%2Fescape/" + posterId).found());

    const std::string unsupportedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            localRecording,
            unsupported);
    assert(!localService.loadPath(unsupportedUrl).found());

    const std::string escapedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            localRecording,
            escaped);
    assert(!localService.loadPath(escapedUrl).found());

    const std::string disguisedUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            localRecording,
            disguised);
    assert(!localService.loadPath(disguisedUrl).found());

    VdrRecordingArtworkService relativeRootService(
        repository,
        {{"default", "relative/root"}});
    assert(!relativeRootService.loadPath(posterUrl).found());

    VdrRecordingArtworkService sizeLimitedService(
        repository,
        {{"default", testRoot.string()}},
        4);
    assert(!sizeLimitedService.loadPath(posterUrl).found());

    std::filesystem::remove_all(testRoot, error);
    std::filesystem::remove_all(remoteRoot, error);
    std::filesystem::remove_all(outsideRoot, error);
    std::remove(databasePath);

    std::cout
        << "test_vdr_recording_artwork_service passed"
        << std::endl;
    return 0;
}
