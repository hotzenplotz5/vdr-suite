#include "Database.h"
#include "RecordingArtworkHttpServer.h"
#include "VdrRecordingArtworkIdentity.h"
#include "VdrRecordingCacheRepository.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace
{

class AuthenticationBoundaryServer : public IHttpServer
{
public:
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

    const std::string pngBytes("\x89PNG\r\n", 6);
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

    RecordingArtworkHttpServer server(
        std::make_unique<AuthenticationBoundaryServer>(),
        repository,
        {root.string()});

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

    std::filesystem::remove_all(root, error);
    std::remove(databasePath);

    return 0;
}
