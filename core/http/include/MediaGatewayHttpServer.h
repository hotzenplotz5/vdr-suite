#pragma once

#include "IHttpServer.h"

#include <memory>
#include <string>

class MediaAccessGrantAuthenticator;
class MediaHlsArtifactReader;
class MediaRouteLeaseRepository;

class MediaGatewayHttpServer : public IHttpServer
{
public:
    MediaGatewayHttpServer(
        std::unique_ptr<IHttpServer> inner,
        const MediaAccessGrantAuthenticator& authenticator,
        const MediaRouteLeaseRepository& routeLeaseRepository,
        const MediaHlsArtifactReader& artifactReader);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    std::unique_ptr<IHttpServer> inner_;
    const MediaAccessGrantAuthenticator& authenticator_;
    const MediaRouteLeaseRepository& routeLeaseRepository_;
    const MediaHlsArtifactReader& artifactReader_;
};