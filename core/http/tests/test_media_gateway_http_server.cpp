#include "MediaGatewayHttpServer.h"

#include "Database.h"
#include "MediaAccessGrantAuthenticator.h"
#include "MediaHlsArtifactReader.h"
#include "MediaRouteLeaseRepository.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

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

MediaSessionIssuanceRequest recordingRequest()
{
    MediaSessionIssuanceRequest value;
    value.actorId = "actor-1";
    value.backendId = "default";
    value.resourceKind = "recording";
    value.resourceId = "42";
    value.presentationProfileId = "hls-fmp4";
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

    MediaAccessGrantAuthenticator authenticator(sessionRepository, 300, 60);
    MediaHlsArtifactReader artifactReader(root.string());
    MediaGatewayHttpServer gateway(
        std::make_unique<FallbackServer>(),
        authenticator,
        routeRepository,
        artifactReader,
        root.string());

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
    live.session.clearSecret();
    return 0;
}
