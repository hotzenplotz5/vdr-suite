#include "TestHttpServer.h"

#include "ApiRouter.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

#include "TestHttpServerPaths.inc"
#include "TestHttpServerAssets.inc"
#include "TestHttpServerRoutes.inc"

}

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter)
{
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!isAuthorized(request))
    {
        return makeUnauthorizedResponse();
    }

    if (request.method == "GET" &&
        isFrontendPath(request.path))
    {
        return serveFrontendPath(request.path);
    }

    if (request.method == "GET" &&
        isChannelLogoPath(request.path))
    {
        return makeChannelLogoResponse(request.path);
    }

    ApiResponse apiResponse;

    if (request.method == "GET")
    {
        apiResponse = apiRouter_.handleClientGet(request.path);
    }
    else if (request.method == "POST")
    {
        apiResponse = apiRouter_.handleClientPost(
            request.path,
            request.body);
    }
    else
    {
        return mapApiResponse(
            405,
            "application/json",
            "{\"error\":\"method not allowed\"}");
    }

    return mapApiResponse(
        apiResponse.statusCode,
        apiResponse.contentType,
        apiResponse.body);
}

HttpServerResponse TestHttpServer::mapApiResponse(
    int statusCode,
    const std::string& contentType,
    const std::string& body) const
{
    HttpServerResponse response;

    response.statusCode = statusCode;
    response.headers["Content-Type"] = contentType;
    response.body = body;

    return response;
}
