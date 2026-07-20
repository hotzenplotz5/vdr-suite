#ifndef TEST_HTTP_SERVER_H
#define TEST_HTTP_SERVER_H

#include "ApiRouter.h"
#include "IEpgArtworkHttpProvider.h"
#include "IHttpServer.h"

class TestHttpServer : public IHttpServer, public IEpgArtworkHttpProvider {
public:
    explicit TestHttpServer(ApiRouter& apiRouter);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

    HttpServerResponse getEpgArtwork(
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId) const override
    {
        const ApiResponse response = apiRouter_.getEpgArtwork(
            backendId,
            channelId,
            eventId);

        return mapApiResponse(
            response.statusCode,
            response.contentType,
            response.body);
    }

private:
    ApiRouter& apiRouter_;

    HttpServerResponse mapApiResponse(
        int statusCode,
        const std::string& contentType,
        const std::string& body) const;
};

#endif
