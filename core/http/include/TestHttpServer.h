#ifndef TEST_HTTP_SERVER_H
#define TEST_HTTP_SERVER_H

#include "IHttpServer.h"

class ApiRouter;

class TestHttpServer : public IHttpServer {
public:
    explicit TestHttpServer(ApiRouter& apiRouter);

    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override;

private:
    ApiRouter& apiRouter_;

    HttpServerResponse mapApiResponse(
        int statusCode,
        const std::string& contentType,
        const std::string& body) const;
};

#endif
