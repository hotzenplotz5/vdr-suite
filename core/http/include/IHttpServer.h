#ifndef I_HTTP_SERVER_H
#define I_HTTP_SERVER_H

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"

class IHttpServer {
public:
    virtual ~IHttpServer() = default;

    virtual HttpServerResponse handleRequest(
        const HttpServerRequest& request) const = 0;
};

#endif
