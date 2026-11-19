#include "Database.h"
#include "MediaProcessRunner.h"
#include "RecordingArtworkHttpServer.h"
#include "ArtworkCacheValidation.h"
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

class AuthenticationBoundaryServer : public IHttpServer, public IEpgArtworkHttpProvider
{
public:
    std::string metadataImage;
    std::string cacheScope;
    HttpServerResponse getEpgArtwork(const std::string& backend,
        const std::string& channel, const std::string& event) const override
    {
        HttpServerResponse response;
        if (backend != "default" || channel != "C1" || event != "42")
        {
            response.statusCode = 404;
            return response;
        }
        response.statusCode = 200;
        response.headers["Content-Type"] = "image/png";
        response.body = metadataImage;
        return response;
    }
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

        if (request.path.find("backend=forbidden") != std::string::npos)
        {
            response.statusCode = 403;
            response.body = "forbidden";
            return response;
        }
        if (request.path.find("/api/vdr/recordings/metadata/image?") == 0 ||
            request.path.find("/api/recordings/metadata/image?") == 0 ||
            request.path.find("/api/epg/cache/metadata/image?backend=default") == 0 ||
            request.path.find("/api/epg/cache/artwork?backend=default&channelId=C2") == 0 ||
            request.path.find("/api/backends/default/settings/series-artwork/image?") == 0 ||
            request.path.find("/api/backends/default/settings/series-artwork/candidate-image?") == 0)
        {
            response.statusCode = 200;
            response.headers["Content-Type"] = "image/png";
            response.headers["Content-Length"] = std::to_string(metadataImage.size());
            response.headers["ETag"] = "original-tag";
            response.headers["Last-Modified"] = "original-date";
            if (!cacheScope.empty()) response.headers["X-VDR-Suite-Artwork-Scope"] = cacheScope;
            response.body = metadataImage;
            return response;
        }
        response.statusCode = 404;
        response.headers["Content-Type"] =
            "application/json";
        if (!cacheScope.empty() &&
            ((request.path.find("/recording-artwork/default/") == 0 &&
              request.path.find("?variant=home") != std::string::npos) ||
             request.path.find("/api/epg/cache/artwork?backend=default&channelId=C1") == 0))
            response.headers["X-VDR-Suite-Artwork-Scope"] = cacheScope;
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

    const auto large = MediaProcessRunner{}.runAndCapture({"/usr/bin/ffmpeg", "-nostdin", "-v", "error",
        "-f", "lavfi", "-i", "testsrc2=size=800x1200", "-frames:v", "1", "-threads", "1",
        "-c:v", "png", "-f", "image2pipe", "pipe:1"}, "/", std::chrono::seconds(10), 16 * 1024 * 1024);
    assert(large.success);
    { std::ofstream file(root / "movies/7/poster.png", std::ios::binary); file << large.output; }
    metadataDelegate->metadataImage = large.output;
    for (const std::string& url : {artworkUrl,
            std::string("/api/vdr/recordings/metadata/image?recordingId=7&kind=preferred"),
            std::string("/api/recordings/metadata/image?recordingId=7&kind=preferred&assignmentRevision=9"),
            std::string("/api/epg/cache/metadata/image?backend=default&channelId=C1&eventId=42&kind=gallery&index=0"),
            std::string("/api/epg/cache/artwork?backend=default&channelId=C1&eventId=42"),
            std::string("/api/epg/cache/artwork?backend=default&channelId=C2&eventId=42")}) {
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
        assert(resized.headers.count("Last-Modified") == 0);
        // Compatibility previews are still delivered, but never persisted
        // by an HTTP cache without an authenticated current scope.
        assert(resized.headers.at("Cache-Control") == "no-store");
        assert(server.handleRequest(authorized).body == resized.body);
        unauthorized.path = authorized.path;
        assert(server.handleRequest(unauthorized).statusCode == 401);
        for (const auto* invalid : {"", "full", "999999"}) {
            authorized.path = variantPrefix + "variant=" + invalid;
            assert(server.handleRequest(authorized).statusCode == 400);
        }
    }
    // Both the scoped tag and the final representation must match. Authorization
    // still runs every time through the original delegate.
    using vdrsuite::http::artworkCacheScope;
    metadataDelegate->cacheScope = artworkCacheScope("actor-a", "default", {"recordings.view"});
    const auto scopeA = metadataDelegate->cacheScope;
    assert(scopeA != artworkCacheScope("actor-b", "default", {"recordings.view"}));
    assert(scopeA != artworkCacheScope("actor-a", "other", {"recordings.view"}));
    assert(scopeA != artworkCacheScope("actor-a", "default", {"role.admin"}));
    authorized.path = "/api/vdr/recordings/metadata/image?backend=default&recordingId=7&kind=preferred&variant=home";
    const auto scoped = server.handleRequest(authorized);
    assert(scoped.statusCode == 200 && scoped.headers.at("Cache-Control") == "private, no-cache");
    const auto scopedTag = scoped.headers.at("ETag");
    authorized.headers["if-none-match"] = scopedTag;
    const auto notModified = server.handleRequest(authorized);
    assert(notModified.statusCode == 304 && notModified.body.empty());
    assert(notModified.headers.at("ETag") == scopedTag);
    metadataDelegate->cacheScope = artworkCacheScope("actor-b", "default", {"recordings.view"});
    assert(server.handleRequest(authorized).statusCode == 200);
    metadataDelegate->cacheScope = scopeA;
    unauthorized.path = authorized.path;
    unauthorized.headers["If-None-Match"] = scopedTag;
    assert(server.handleRequest(unauthorized).statusCode == 401);
    authorized.headers.erase("if-none-match");
    authorized.path = artworkUrl + "?variant=home";
    const auto directScoped = server.handleRequest(authorized);
    assert(directScoped.statusCode == 200);
    assert(directScoped.headers.at("X-VDR-Suite-Artwork-Scope") == scopeA);
    assert(directScoped.headers.at("Cache-Control") == "private, no-cache");
    const auto directTag = directScoped.headers.at("ETag");
    authorized.headers["If-None-Match"] = directTag;
    assert(server.handleRequest(authorized).statusCode == 304);
    metadataDelegate->cacheScope = artworkCacheScope("actor-b", "default", {"recordings.view"});
    assert(server.handleRequest(authorized).statusCode == 200);
    metadataDelegate->cacheScope = scopeA;
    unauthorized.path = authorized.path;
    unauthorized.headers["If-None-Match"] = directTag;
    assert(server.handleRequest(unauthorized).statusCode == 401);
    authorized.path = artworkUrl;
    assert(server.handleRequest(authorized).headers.count("X-VDR-Suite-Artwork-Scope") == 0);
    authorized.path = missing.path + "?variant=home";
    const auto directMissing = server.handleRequest(authorized);
    assert(directMissing.statusCode == 404);
    assert(directMissing.headers.count("X-VDR-Suite-Artwork-Scope") == 0);
    authorized.path = artworkUrl + "?variant=home";
    metadataDelegate->cacheScope.clear();
    const auto directUnscoped = server.handleRequest(authorized);
    assert(directUnscoped.statusCode == 200);
    assert(directUnscoped.headers.at("Cache-Control") == "no-store");
    assert(directUnscoped.headers.count("X-VDR-Suite-Artwork-Scope") == 0);
    authorized.path = "/api/vdr/recordings/metadata/image?backend=default&recordingId=7&kind=preferred&variant=home";
    authorized.headers["If-None-Match"] = scopedTag;
    const auto unscoped = server.handleRequest(authorized);
    assert(unscoped.statusCode == 200 && unscoped.headers.at("Cache-Control") == "no-store");
    assert(unscoped.headers.count("X-VDR-Suite-Artwork-Scope") == 0);
    authorized.headers.erase("If-None-Match");

    // The existing selected series route uses the same final-byte
    // decorator. Candidate images remain full-size and are not revalidated.
    metadataDelegate->cacheScope = scopeA;
    authorized.path = "/api/backends/default/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7&variant=home";
    const auto seriesPreview = server.handleRequest(authorized);
    assert(seriesPreview.statusCode == 200);
    assert(seriesPreview.body.size() < large.output.size());
    assert(seriesPreview.headers.at("X-VDR-Suite-Artwork-Scope") == scopeA);
    const auto seriesTag = seriesPreview.headers.at("ETag");
    authorized.headers["If-None-Match"] = seriesTag;
    assert(server.handleRequest(authorized).statusCode == 304);
    metadataDelegate->cacheScope = artworkCacheScope("actor-b", "default", {"recordings.view"});
    assert(server.handleRequest(authorized).statusCode == 200);
    unauthorized.path = authorized.path;
    unauthorized.headers["If-None-Match"] = seriesTag;
    assert(server.handleRequest(unauthorized).statusCode == 401);
    metadataDelegate->cacheScope.clear();
    const auto seriesUnscoped = server.handleRequest(authorized);
    assert(seriesUnscoped.statusCode == 200);
    assert(seriesUnscoped.headers.at("Cache-Control") == "no-store");
    authorized.headers.erase("If-None-Match");
    authorized.path = "/api/backends/default/settings/series-artwork/candidate-image?externalId=123&posterReference=%2Fposter.jpg&variant=home";
    assert(server.handleRequest(authorized).body == large.output);
    authorized.path = "/api/backends/default/settings/series-artwork/image?seriesKey=folder%3Aseries&revision=7";
    assert(server.handleRequest(authorized).body == large.output);

    // Synthetic final bytes exercise the validator without resizing, both
    // delegated and compatibility routes, above the recording preview cap.
    metadataDelegate->cacheScope = artworkCacheScope("actor-a", "default", {"epg.view"});
    metadataDelegate->metadataImage = std::string(414U * 1024U, 'x');
    for (const auto* url : {
        "/api/epg/cache/artwork?backend=default&channelId=C1&eventId=42",
        "/api/epg/cache/artwork?backend=default&channelId=C2&eventId=42",
        "/api/epg/cache/metadata/image?backend=default&channelId=C1&eventId=42&kind=preferred&index=0"}) {
        authorized.path = url;
        const auto hero = server.handleRequest(authorized);
        assert(hero.statusCode == 200 && hero.body == metadataDelegate->metadataImage);
        assert(hero.headers.at("X-VDR-Suite-Artwork-Bytes") == std::to_string(hero.body.size()));
        assert(hero.headers.at("Cache-Control") == "private, no-cache");
        authorized.headers["If-None-Match"] = hero.headers.at("ETag");
        const auto reused = server.handleRequest(authorized);
        assert(reused.statusCode == 304 && reused.body.empty());
        unauthorized.path = url;
        unauthorized.headers["If-None-Match"] = hero.headers.at("ETag");
        assert(server.handleRequest(unauthorized).statusCode == 401);
        authorized.headers.erase("If-None-Match");
    }
    metadataDelegate->metadataImage = std::string(1024U * 1024U + 1U, 'x');
    const auto oversizedHero = server.handleRequest(authorized);
    assert(oversizedHero.statusCode == 200 && oversizedHero.body == metadataDelegate->metadataImage);
    assert(oversizedHero.headers.count("X-VDR-Suite-Artwork-Scope") == 0);
    metadataDelegate->metadataImage = large.output;
    metadataDelegate->cacheScope.clear();

    for (const auto* route : {"/api/epg/cache/artwork", "/api/epg/cache/metadata/image"})
    {
        authorized.path = std::string(route) + "?backend=forbidden&channelId=C1&eventId=42&variant=home";
        assert(server.handleRequest(authorized).statusCode == 403);
        authorized.path = std::string(route) + "?backend=missing&channelId=C1&eventId=42&variant=home";
        assert(server.handleRequest(authorized).statusCode == 404);
    }
    authorized.path = "/recording-artwork/default/..%2fsecret?variant=home";
    assert(server.handleRequest(authorized).statusCode == 404);

    std::filesystem::remove_all(root, error);
    std::remove(databasePath);

    return 0;
}
