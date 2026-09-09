#include "Database.h"
#include "MediaProcessRunner.h"
#include "RecordingArtworkHttpServer.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>

namespace
{

class AuthenticationBoundaryServer : public IHttpServer
{
public:
    std::string metadataImage;
    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override
    {
        HttpServerResponse response;

        const auto authorization =
            request.headers.find("Authorization");

        if (authorization == request.headers.end() ||
            authorization->second != "Basic accepted")
        {
            response.statusCode = 401;
            response.headers["Content-Type"] =
                "application/json";
            response.body = "{\"error\":\"authentication required\"}";
            return response;
        }

        if (request.method != "GET")
        {
            response.statusCode = 405;
            response.headers["Content-Type"] =
                "application/json";
            response.body = "{\"error\":\"method not allowed\"}";
            return response;
        }

        if (request.path.find("/api/vdr/recordings/metadata/image?") == 0 ||
            request.path.find("/api/recordings/metadata/image?") == 0)
        {
            response.statusCode = 200;
            response.headers["Content-Type"] = "image/png";
            response.headers["Content-Length"] = std::to_string(metadataImage.size());
            response.headers["ETag"] = "original-tag";
            response.body = metadataImage;
            return response;
        }
        response.statusCode = 404;
        response.headers["Content-Type"] =
            "application/json";
        response.body = "{\"error\":\"route not found\"}";
        return response;
    }
};

VdrRecording makeRecording()
{
    VdrRecording recording;
    recording.id = "7";
    recording.backendId = "default";
    recording.backendNativeId =
        "/srv/vdr/video/Movies/Zero/recording.rec";
    recording.path = "/Movies/Zero/recording.rec";
    recording.title = "Zero";

    VdrRecordingArtworkRef artwork;
    artwork.kind = VdrRecordingArtworkKind::Poster;
    artwork.source =
        VdrRecordingMetadataSource::RestfulApiScraperBridge;
    artwork.reference = "movies/7/poster.png";
    recording.metadata.artwork.push_back(artwork);

    return recording;
}

}

int main()
{
    const std::filesystem::path root =
        "/tmp/vdr-suite-artwork-http-root";
    const char* databasePath =
        "/tmp/test_recording_artwork_http_server.db";

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::remove(databasePath);
    std::filesystem::create_directories(root / "movies/7");

    const char pngData[] = {
        static_cast<char>(0x89),
        'P',
        'N',
        'G',
        '\r',
        '\n',
        static_cast<char>(0x1a),
        '\n'};
    const std::string pngBytes(pngData, sizeof(pngData));
    {
        std::ofstream file(
            root / "movies/7/poster.png",
            std::ios::binary);
        assert(file.good());
        file.write(
            pngBytes.data(),
            static_cast<std::streamsize>(pngBytes.size()));
        assert(file.good());
    }

    Database database;
    assert(database.open(databasePath));
    VdrRecordingCacheRepository repository(database);
    assert(repository.ensureSchema());

    const VdrRecording recording = makeRecording();
    assert(repository.replaceRecordingsForBackend(
        "default",
        {recording}));

    const std::string artworkUrl =
        VdrRecordingArtworkIdentity::publicUrl(
            recording,
            recording.metadata.artwork.front());

    auto delegate = std::make_unique<AuthenticationBoundaryServer>();
    auto* metadataDelegate = delegate.get();
    RecordingArtworkHttpServer server(
        std::move(delegate),
        repository,
        {{"default", root.string()}}, (root / "previews").string());

    HttpServerRequest unauthorized;
    unauthorized.method = "GET";
    unauthorized.path = artworkUrl;

    const HttpServerResponse unauthorizedResponse =
        server.handleRequest(unauthorized);
    assert(unauthorizedResponse.statusCode == 401);
    assert(unauthorizedResponse.body.find("authentication") !=
           std::string::npos);

    HttpServerRequest authorized = unauthorized;
    authorized.headers["Authorization"] = "Basic accepted";

    const HttpServerResponse artworkResponse =
        server.handleRequest(authorized);
    assert(artworkResponse.statusCode == 200);
    assert(artworkResponse.headers.at("Content-Type") ==
           "image/png");
    assert(artworkResponse.headers.at("Cache-Control") ==
           "private, max-age=300");
    assert(artworkResponse.headers.at("X-Content-Type-Options") ==
           "nosniff");
    assert(artworkResponse.body == pngBytes);

    HttpServerRequest missing = authorized;
    missing.path =
        "/recording-artwork/default/"
        "00000000000000000000000000000000";

    const HttpServerResponse missingResponse =
        server.handleRequest(missing);
    assert(missingResponse.statusCode == 404);
    assert(missingResponse.headers.at("Cache-Control") ==
           "no-store");
    assert(missingResponse.headers.at("X-Content-Type-Options") ==
           "nosniff");
    assert(missingResponse.body.find("recording artwork not found") !=
           std::string::npos);

    HttpServerRequest post = authorized;
    post.method = "POST";

    const HttpServerResponse postResponse =
        server.handleRequest(post);
    assert(postResponse.statusCode == 405);

    const auto large = MediaProcessRunner{}.runAndCapture({"/usr/bin/ffmpeg", "-v", "error",
        "-f", "lavfi", "-i", "testsrc2=size=800x1200", "-frames:v", "1", "-threads", "1",
        "-c:v", "png", "-f", "image2pipe", "pipe:1"}, "/", std::chrono::seconds(10), 16 * 1024 * 1024);
    assert(large.success);
    { std::ofstream file(root / "movies/7/poster.png", std::ios::binary); file << large.output; }
    metadataDelegate->metadataImage = large.output;
    for (const std::string& url : {artworkUrl,
            std::string("/api/vdr/recordings/metadata/image?recordingId=7&kind=preferred"),
            std::string("/api/recordings/metadata/image?recordingId=7&kind=preferred&assignmentRevision=9")}) {
        authorized.path = url;
        const auto original = server.handleRequest(authorized);
        assert(original.statusCode == 200 && original.body == large.output);
        authorized.path += (url.find('?') == std::string::npos ? "?" : "&");
        const auto variantPrefix = authorized.path;
        authorized.path += "variant=home";
        const auto resized = server.handleRequest(authorized);
        assert(resized.statusCode == 200 && resized.body.size() < original.body.size());
        assert(resized.headers.at("Content-Type") == "image/jpeg");
        assert(resized.headers.count("Content-Length") == 0 && resized.headers.count("ETag") == 0);
        assert(resized.headers.at("Cache-Control") == original.headers.at("Cache-Control"));
        assert(server.handleRequest(authorized).body == resized.body);
        unauthorized.path = authorized.path;
        assert(server.handleRequest(unauthorized).statusCode == 401);
        for (const auto* invalid : {"", "full", "999999"}) {
            authorized.path = variantPrefix + "variant=" + invalid;
            assert(server.handleRequest(authorized).statusCode == 400);
        }
    }
    authorized.path = "/recording-artwork/default/..%2fsecret?variant=home";
    assert(server.handleRequest(authorized).statusCode == 404);

    std::filesystem::remove_all(root, error);
    std::remove(databasePath);

    return 0;
}
