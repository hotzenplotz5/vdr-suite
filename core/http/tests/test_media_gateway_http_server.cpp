#include "MediaGatewayHttpServer.h"

#include "Database.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "RecordingDirectSourceRegistry.h"
#include "RecordingSourceFingerprint.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace
{

class FallbackServer : public IHttpServer
{
public:
    HttpServerResponse handleRequest(const HttpServerRequest&) const override
    {
        HttpServerResponse response;
        response.statusCode = 418;
        response.body = "fallback";
        return response;
    }
};

class DeterministicEntropy
{
public:
    bool fill(unsigned char* output, std::size_t size)
    {
        if (output == nullptr || size == 0) return false;
        for (std::size_t index = 0; index < size; ++index) output[index] = next_++;
        return true;
    }
private:
    unsigned char next_ = 1;
};

std::chrono::system_clock::time_point futureClock()
{
    std::tm utc{};
    utc.tm_year = 130;
    utc.tm_mon = 0;
    utc.tm_mday = 1;
    return std::chrono::system_clock::from_time_t(timegm(&utc));
}

void writeFile(const std::filesystem::path& path, const std::string& value)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
}

MediaSessionIssuanceRequest recordingRequest(const std::string& profile = "hls-fmp4")
{
    MediaSessionIssuanceRequest value;
    value.actorId = "actor-1";
    value.backendId = "default";
    value.resourceKind = "recording";
    value.resourceId = "42";
    value.presentationProfileId = profile;
    value.providerId = "local-vdr-recording";
    value.lifetimeSeconds = 21600;
    return value;
}

MediaSessionIssuanceRequest liveRequest()
{
    MediaSessionIssuanceRequest value;
    value.actorId = "actor-live";
    value.backendId = "default";
    value.resourceKind = "live-channel";
    value.resourceId = "S19.2E-1-1019-10301";
    value.presentationProfileId = "live-progressive-fmp4";
    value.providerId = "suitebridge-native-live";
    value.lifetimeSeconds = 21600;
    return value;
}

} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MediaSessionRepository sessionRepository(database);
    assert(sessionRepository.ensureSchema());
    MediaRouteLeaseRepository routeRepository(database);

    DeterministicEntropy entropy;
    MediaSessionIssuanceService issuer(
        sessionRepository,
        [&entropy](unsigned char* output, std::size_t size) {
            return entropy.fill(output, size);
        },
        [] { return futureClock(); });

    auto issued = issuer.issue(recordingRequest());
    assert(issued.issued);
    assert(sessionRepository.activateBundle(issued.session.sessionId));

    const auto root = std::filesystem::temp_directory_path() /
        "vdr-suite-media-gateway-http-test";
    const auto workspace = root / issued.session.workspaceId;
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(workspace);
    writeFile(workspace / "master.m3u8", "#EXTM3U\nsegment-000001.m4s\n");
    writeFile(workspace / "segment-000001.m4s", "media-bytes");

    const auto directDirectory = root / "direct.rec";
    std::filesystem::create_directories(directDirectory);
    const auto directFirst = directDirectory / "00001.ts";
    const auto directSecond = directDirectory / "00002.ts";
    writeFile(directFirst, "ABCDE");
    writeFile(directSecond, "FGHIJ");
    const std::vector<std::string> directSegments = {
        directFirst.string(), directSecond.string()};
    const RecordingSourceFingerprint directFingerprint =
        inspectRecordingSource(directDirectory.string(), directSegments);
    assert(directFingerprint.valid);

    auto directIssued = issuer.issue(recordingRequest("progressive-direct"));
    assert(directIssued.issued);
    RecordingDirectSourceRegistry directRegistry;
    RecordingDirectSourceRegistration directRegistration;
    directRegistration.recordingDirectory = directDirectory.string();
    directRegistration.segmentPaths = directSegments;
    directRegistration.sourceFingerprint = directFingerprint.value;
    directRegistration.readableBytes = directFingerprint.readableBytes;
    std::string directFailure;
    assert(directRegistry.registerCompleted(
        directIssued.session.sessionId,
        directRegistration,
        directFailure));
    assert(sessionRepository.activateBundle(directIssued.session.sessionId));

    MediaAccessGrantAuthenticator authenticator(sessionRepository, 300, 60);
    MediaHlsArtifactReader artifactReader(root.string());
    MediaGatewayHttpServer gateway(
        std::make_unique<FallbackServer>(),
        authenticator,
        routeRepository,
        artifactReader,
        root.string(),
        &directRegistry);

    const std::string prefix =
        "/api/media/sessions/" + issued.session.sessionId + "/hls/";

    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = "/api/not-media";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 418);
        assert(response.body == "fallback");
    }

    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = prefix + "master.m3u8";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 401);
        assert(response.body.find("media_access_credential_required") != std::string::npos);
    }

    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = prefix + "master.m3u8";
        http.headers["Cookie"] =
            "other=x; vdr_suite_media=" + issued.session.accessCredential + "; theme=dark";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 200);
        assert(response.headers.at("Content-Type") == "application/vnd.apple.mpegurl");
        assert(response.body == "#EXTM3U\nsegment-000001.m4s\n");
        assert(response.headers.at("Cross-Origin-Resource-Policy") == "same-origin");
        assert(response.streamBodyPath.empty());
    }

    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = prefix + "segment-000001.m4s";
        http.headers["X-VDR-Suite-Media-Authorization"] =
            "Bearer " + issued.session.accessCredential;
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 200);
        assert(response.headers.at("Content-Type") == "video/iso.segment");
        assert(response.body == "media-bytes");
    }

    const std::string directPath =
        "/api/media/sessions/" + directIssued.session.sessionId +
        "/recording/stream.ts";
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Range"] = "bytes=0-4";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 401);
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 400);
        assert(response.headers.at("Accept-Ranges") == "bytes");
        assert(response.body.find("media_byte_range_required") != std::string::npos);
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=3-8";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 206);
        assert(response.headers.at("Content-Type") == "video/mp2t");
        assert(response.headers.at("Accept-Ranges") == "bytes");
        assert(response.headers.at("Content-Range") == "bytes 3-8/10");
        assert(response.body == "DEFGHI");
        assert(response.body.find(directDirectory.string()) == std::string::npos);
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=-3";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 206);
        assert(response.headers.at("Content-Range") == "bytes 7-9/10");
        assert(response.body == "HIJ");
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=10-";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 416);
        assert(response.headers.at("Content-Range") == "bytes */10");
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=0-1,4-5";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 400);
        assert(response.body.find("media_byte_range_invalid") != std::string::npos);
    }

    writeFile(directSecond, "changed");
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=0-1";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 409);
        assert(response.body.find("recording_source_changed") != std::string::npos);
        assert(response.body.find(directDirectory.string()) == std::string::npos);
    }

    assert(sessionRepository.endBundle(directIssued.session.sessionId, "client_stop"));
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = directPath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + directIssued.session.accessCredential;
        http.headers["Range"] = "bytes=0-1";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 401);
        assert(response.body.find("media_access_inactive") != std::string::npos);
    }

    assert(sessionRepository.endBundle(issued.session.sessionId, "client_stop"));
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = prefix + "master.m3u8";
        http.headers["Cookie"] =
            "vdr_suite_media=" + issued.session.accessCredential;
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 401);
        assert(response.body.find("media_access_inactive") != std::string::npos);
    }

    auto live = issuer.issue(liveRequest());
    assert(live.issued);
    assert(sessionRepository.activateBundle(live.session.sessionId));
    const auto liveWorkspace = root / live.session.workspaceId;
    std::filesystem::create_directories(liveWorkspace);
    const auto liveFifo = liveWorkspace / "live.fmp4";
    assert(::mkfifo(liveFifo.c_str(), 0600) == 0);

    const std::string livePath =
        "/api/media/sessions/" + live.session.sessionId + "/live/stream.mp4";
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = livePath;
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 401);
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = livePath;
        http.headers["Cookie"] =
            "vdr_suite_media=" + live.session.accessCredential;
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 200);
        assert(response.headers.at("Content-Type") == "video/mp4");
        assert(response.headers.at("Cache-Control") == "no-store");
        assert(response.headers.at("X-Accel-Buffering") == "no");
        assert(response.body.empty());
        assert(response.streamBodyPath == liveFifo.string());
    }
    {
        HttpServerRequest http;
        http.method = "GET";
        http.path = livePath + "?token=forbidden";
        const auto response = gateway.handleRequest(http);
        assert(response.statusCode == 418);
    }

    assert(sessionRepository.endBundle(live.session.sessionId, "client_stop"));
    std::filesystem::remove_all(root);
    issued.session.clearSecret();
    directIssued.session.clearSecret();
    live.session.clearSecret();
    return 0;
}
