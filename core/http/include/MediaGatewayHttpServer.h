#pragma once

#include "IHttpServer.h"

#include <functional>
#include <memory>
#include <string>

class MediaAccessGrantAuthenticator;
class MediaHlsArtifactReader;
class MediaRouteLeaseRepository;
class RecordingDirectSourceRegistry;

class MediaGatewayHttpServer : public IHttpServer
{
public:
    using PublicSessionAuthorizer =
        std::function<bool(
            const std::string& sessionId,
            const std::string& actorId,
            const std::string& backendId)>;

    MediaGatewayHttpServer(
        std::unique_ptr<IHttpServer> inner,
        const MediaAccessGrantAuthenticator& authenticator,
        const MediaRouteLeaseRepository& routeLeaseRepository,
        const MediaHlsArtifactReader& artifactReader,
        std::string workspaceRoot = "/var/cache/vdr-suite/media-sessions",
        const RecordingDirectSourceRegistry* directSourceRegistry = nullptr,
        PublicSessionAuthorizer publicSessionAuthorizer = {});

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    std::unique_ptr<IHttpServer> inner_;
    const MediaAccessGrantAuthenticator& authenticator_;
    const MediaRouteLeaseRepository& routeLeaseRepository_;
    const MediaHlsArtifactReader& artifactReader_;
    std::string workspaceRoot_;
    const RecordingDirectSourceRegistry* directSourceRegistry_ = nullptr;
    PublicSessionAuthorizer publicSessionAuthorizer_;
};
